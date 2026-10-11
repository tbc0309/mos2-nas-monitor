"""Read-only SNMP v2c probe; Python 3, no external packages."""
import argparse
import secrets
import getpass
import socket
import struct

def tlv(tag, value):
    n = len(value)
    length = bytes([n]) if n < 128 else b'\x82' + struct.pack('>H', n)
    return bytes([tag]) + length + value

def integer(n):
    return tlv(2, n.to_bytes(4, 'big'))

def oid(text):
    parts = list(map(int, text.split('.')))
    b = bytes([parts[0] * 40 + parts[1]])
    for n in parts[2:]:
        chunk = [n & 127]
        n >>= 7
        while n:
            chunk.insert(0, (n & 127) | 128)
            n >>= 7
        b += bytes(chunk)
    return tlv(6, b)

def read(b, offset=0):
    if offset + 2 > len(b):
        raise ValueError('truncated BER')
    tag, n = b[offset:offset+2]
    offset += 2
    if n & 128:
        count = n & 127
        if not count or count > 2 or offset + count > len(b):
            raise ValueError('invalid BER length')
        n = int.from_bytes(b[offset:offset+count], 'big')
        offset += count
    if offset + n > len(b):
        raise ValueError('truncated BER payload')
    return tag, b[offset:offset+n], offset+n

def oid_text(b):
    result = [min(b[0] // 40, 2), b[0] - min(b[0] // 40, 2)*40]
    n = 0
    for v in b[1:]:
        n = (n << 7) | (v & 127)
        if not v & 128:
            result.append(n)
            n = 0
    return '.'.join(map(str, result))

def query(host, port, community, name, next_oid=False, timeout=2):
    ident = secrets.randbelow(0x7ffffffe) + 1
    varbind = tlv(48, oid(name) + tlv(5, b''))
    pdu = tlv(0xa1 if next_oid else 0xa0, integer(ident)+integer(0)+integer(0)+tlv(48,varbind))
    request = tlv(48, integer(1)+tlv(4,community.encode())+pdu)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
        udp.settimeout(timeout)
        udp.connect((host, port))
        udp.send(request)
        packet = udp.recv(1500)
    tag, body, _ = read(packet)
    if tag != 48:
        raise ValueError('invalid response')
    _, version, pos = read(body)
    _, _, pos = read(body, pos)
    tag, pdu, _ = read(body, pos)
    if tag != 0xa2 or int.from_bytes(version, 'big') != 1:
        raise ValueError('not SNMP v2c response')
    _, request_id, pos = read(pdu)
    _, error, pos = read(pdu, pos)
    if int.from_bytes(request_id, 'big') != ident or int.from_bytes(error, 'big'):
        raise ValueError('SNMP request ID or error')
    _, _, pos = read(pdu, pos)
    _, bindings, _ = read(pdu, pos)
    _, binding, _ = read(bindings)
    _, object_id, pos = read(binding)
    tag, value, _ = read(binding, pos)
    if tag >= 0x80:
        raise ValueError('OID unavailable')
    return oid_text(object_id), value.decode('utf8',errors='replace') if tag == 4 else int.from_bytes(value,'big')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host', default='192.168.1.100')
    p.add_argument('--port', type=int, default=161)
    p.add_argument('--community', help='Omit to enter community without echo')
    args = p.parse_args()
    community = args.community or getpass.getpass('SNMP community: ')
    items = {'System': '1.3.6.1.2.1.1.1.0', 'CPU idle %': '1.3.6.1.4.1.2021.11.11.0', 'RAM total KB': '1.3.6.1.4.1.2021.4.5.0', 'RAM available KB': '1.3.6.1.4.1.2021.4.6.0', 'Temperature C': '1.3.6.1.4.1.6574.1.2.0', 'Model': '1.3.6.1.4.1.6574.1.5.1.0'}
    for label, name in items.items():
        try:
            print(label, query(args.host,args.port,community,name)[1])
        except (OSError,ValueError) as e:
            print(label, 'FAILED:', e)
    for label, root in [('Storage indexes','1.3.6.1.2.1.25.2.3.1.3'),('Interface indexes','1.3.6.1.2.1.2.2.1.2')]:
        print(label)
        current = root
        for _ in range(64):
            try:
                name,value = query(args.host,args.port,community,current,True)
                if not name.startswith(root+'.') or name == current:
                    break
                print(' ',name.rsplit('.',1)[-1], value)
                current = name
            except (OSError,ValueError) as e:
                print('  FAILED:',e)
                break

if __name__ == '__main__':
    main()
