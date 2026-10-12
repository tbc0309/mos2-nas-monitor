"""Generic-first SNMP readers. Transport errors never change capabilities.

The query callback owns rate limiting, authentication and timeout policy.
Only UnsupportedOid (a decoded unsupported object) selects another metric path.
Keep this policy aligned with include/snmp; tests cover both implementations.
"""
import time
import re
from snmp_errors import UnsupportedOid

SYSTEM_DESCRIPTION = '1.3.6.1.2.1.1.1.0'
SYSTEM_OBJECT_ID = '1.3.6.1.2.1.1.2.0'
SYSTEM_NAME = '1.3.6.1.2.1.1.5.0'
VENDOR_MODEL = '1.3.6.1.4.1.6574.1.5.1.0'


def subtree(oid, root):
    return oid == root or oid.startswith(root + '.')


def temperature_rank(label):
    label = str(label)[:96].lower()
    if any(word in label for word in ('disk', 'drive', 'nvme', 'ssd', 'hdd')):
        return 0
    if any(word in label for word in ('package', 'tdie', 'tctl', 'cpu', 'soc_dts')):
        return 100
    if 'core' in label:
        return 70
    if any(word in label for word in ('system', 'board', 'ambient')):
        return 30
    return 10


def classify(description, object_id, verified_model=False):
    if verified_model or subtree(object_id, '1.3.6.1.4.1.6574'):
        return 'synology'
    if any(subtree(object_id, root) for root in ('1.3.6.1.4.1.24681', '1.3.6.1.4.1.55062')):
        return 'qnap'
    for token in ('synology', 'openwrt', 'ubuntu', 'fnos', 'terramaster', 'qnap', 'linux'):
        if token in description.lower():
            return token
    return 'generic' if description or object_id else 'unknown'


class Profile:
    def __init__(self):
        self.system = 'unknown'
        self.model = ''
        self.host_name = ''
        self.vendor_model_unsupported = False
        self.vendor_verified = False
        self.synology_confirmed = False
        self.cpu_mode = 'raw'
        self.memory_mode = 'available'
        self.network_mode = '64'
        self.temperature_mode = 'sensor'
        self.sensor_oid = ''
        self.cpu_previous = None
        self.network_previous = None

    def identity(self, read):
        values = []
        for oid in (SYSTEM_DESCRIPTION, SYSTEM_OBJECT_ID):
            try:
                value=read(oid)[1]
                if not isinstance(value,str): raise ValueError('Invalid system identity type')
                values.append(value)
            except UnsupportedOid:
                values.append('')
        description, object_id = values
        name = ''
        match = re.match(r'^(?:Linux|FreeBSD|OpenBSD|NetBSD|Darwin|SunOS) ([A-Za-z0-9_.-]{1,64}) ', description)
        self.host_name = match.group(1) if match and all(32 <= ord(c) <= 126 for c in description) else ''
        if not self.host_name:
            try:
                fallback = read(SYSTEM_NAME)[1]
                self.host_name = fallback if isinstance(fallback,str) and re.fullmatch(r'[A-Za-z0-9_.-]{1,64}',fallback) else ''
            except UnsupportedOid:
                pass
        verified = False
        # Publish standard identity evidence before any optional vendor request.
        # A slow model OID must not postpone known enterprise capabilities.
        self.system = classify(description, object_id, self.vendor_verified)
        if subtree(object_id, '1.3.6.1.4.1.6574') and not self.synology_confirmed:
            self.synology_confirmed = True
            self.memory_mode, self.temperature_mode = 'free_cache', 'synology'
        if not self.vendor_model_unsupported and (
            classify(description, object_id) in ('synology', 'linux')
            or subtree(object_id, '1.3.6.1.4.1.8072.3.2.10')
        ):
            try:
                name = read(VENDOR_MODEL)[1]
                if not isinstance(name,str): raise ValueError('Invalid vendor model type')
                verified = bool(name)
            except UnsupportedOid:
                self.vendor_model_unsupported = True
        self.vendor_verified |= verified
        confirmed=self.vendor_verified or subtree(object_id,'1.3.6.1.4.1.6574')
        system = classify(description, object_id, self.vendor_verified)
        if confirmed and not self.synology_confirmed:
            self.memory_mode = 'free_cache'
            self.temperature_mode = 'synology'
        self.synology_confirmed=confirmed
        self.system, self.model = system, name[:64]
        return self.model

    def automatic_name(self):
        kind = self.model if self.vendor_verified and self.model else {
            'synology':'Synology', 'ubuntu':'Ubuntu', 'openwrt':'OpenWrt',
            'fnos':'fnOS', 'terramaster':'TOS', 'qnap':'QNAP', 'linux':'Linux',
        }.get(self.system, 'SNMP')
        return f"{self.host_name or 'SNMP'} ({kind})"

    def cpu(self, read):
        if self.cpu_mode == 'unsupported':
            return None
        if self.cpu_mode == 'raw':
            try:
                values = [int(read(f'1.3.6.1.4.1.2021.11.{n}.0')[1]) for n in (50,51,52,53,54,56,61,64)]
                result = None
                if self.cpu_previous is not None:
                    delta = [(a-b) & 0xffffffff for a,b in zip(values,self.cpu_previous)]
                    if sum(delta) and all(n <= 0x7fffffff for n in delta):
                        result = 100 * (sum(delta)-delta[3]-delta[4])/sum(delta)
                self.cpu_previous = values
                return result
            except UnsupportedOid:
                self.cpu_mode = 'processor_table'
                self.cpu_previous = None
        root = '1.3.6.1.2.1.25.3.3.1.2'
        values, cursor = [], root
        for _ in range(32):
            try:
                oid, value = read(cursor, True)
            except UnsupportedOid:
                break
            if oid == cursor: raise ValueError('Processor walk did not advance')
            if not subtree(oid,root):
                break
            if not 0 <= int(value) <= 100:
                raise ValueError('Invalid processor load')
            values.append(int(value)); cursor = oid
        if values:
            return sum(values)/len(values)
        self.cpu_mode = 'unsupported'
        return None

    def memory(self, read):
        while self.memory_mode != 'unsupported':
            try:
                if self.memory_mode != 'host_ram':
                    total = int(read('1.3.6.1.4.1.2021.4.5.0')[1])
                    if self.memory_mode == 'available':
                        available = int(read('1.3.6.1.4.1.2021.4.27.0')[1])
                    else:
                        available = min(total,sum(int(read(f'1.3.6.1.4.1.2021.4.{n}.0')[1]) for n in (6,14,15)))
                    if total <= 0 or not 0 <= available <= total:
                        raise ArithmeticError('Invalid memory snapshot')
                    return total*1024, available*1024
                root = '1.3.6.1.2.1.25.2.3.1.2'; cursor = root
                for _ in range(64):
                    oid, kind = read(cursor,True)
                    if oid == cursor: raise ValueError('RAM walk did not advance')
                    if not subtree(oid,root):
                        break
                    cursor = oid
                    if kind == '1.3.6.1.2.1.25.2.1.2':
                        index = oid.rsplit('.',1)[1]
                        units,size,used = [int(read(f'1.3.6.1.2.1.25.2.3.1.{column}.{index}')[1]) for column in (4,5,6)]
                        if units <= 0 or size <= 0 or not 0 <= used <= size:
                            raise ArithmeticError('Invalid host RAM')
                        return units*size, units*(size-used)
                self.memory_mode = 'unsupported'
            except UnsupportedOid:
                self.memory_mode = {'available':'free_cache','free_cache':'host_ram','host_ram':'unsupported'}[self.memory_mode]
        return None

    def temperature(self, read):
        if self.temperature_mode == 'unsupported':
            return None
        if self.temperature_mode == 'synology':
            try:
                value = float(read('1.3.6.1.4.1.6574.1.2.0')[1])
                return value if 0 <= value <= 150 else None
            except UnsupportedOid:
                self.temperature_mode = 'sensor'
        if self.temperature_mode == 'cpu_extension':
            try:
                value=read('1.3.6.1.4.1.8072.1.3.2.3.1.1.11.109.111.115.50.67.112.117.84.101.109.112')[1]
                if not isinstance(value,str) or not value.isascii() or not value.isdigit() or len(value)>6: raise ValueError('Invalid CPU temperature')
                value=int(value)/1000
                return value if 0<=value<=150 else None
            except UnsupportedOid:
                self.temperature_mode='unsupported';return None
        root = '1.3.6.1.4.1.2021.13.16.2.1.3'
        if self.sensor_oid:
            try:
                value = float(read(self.sensor_oid)[1]) / 1000
                if not 0 <= value <= 150: raise ValueError('Invalid cached sensor temperature')
                return value
            except UnsupportedOid:
                self.sensor_oid = ''
        cursor, best = root, None
        for _ in range(16):
            try:
                oid, value = read(cursor, True)
            except UnsupportedOid:
                break
            if oid == cursor: raise ValueError('Sensor walk did not advance')
            if not subtree(oid, root): break
            cursor = oid
            if not isinstance(value, (int, float)) or not 0 <= value <= 150000: continue
            try:
                label = read('1.3.6.1.4.1.2021.13.16.2.1.2.' + oid.rsplit('.', 1)[1])[1]
            except UnsupportedOid:
                label = ''
            rank = temperature_rank(label)
            if rank and (best is None or (rank, value) > best[:2]): best = (rank, value, oid)
            if rank >= 100: break
        if best:
            self.sensor_oid = best[2]
            return best[1] / 1000
        self.temperature_mode = 'cpu_extension'
        return self.temperature(read)

    def network(self, read, index):
        while self.network_mode != 'unsupported':
            try:
                roots = ('1.3.6.1.2.1.31.1.1.1.6','1.3.6.1.2.1.31.1.1.1.10') if self.network_mode == '64' else ('1.3.6.1.2.1.2.2.1.10','1.3.6.1.2.1.2.2.1.16')
                values = [int(read(root+'.'+index)[1]) for root in roots]
                now = time.monotonic(); previous = self.network_previous
                self.network_previous = values,now,index
                if previous and previous[2] == index and now > previous[1]:
                    delta = [a-b for a,b in zip(values,previous[0])]
                    if self.network_mode == '32':
                        delta = [n & 0xffffffff for n in delta]
                    if all(n >= 0 and (self.network_mode == '64' or n <= 0x7fffffff) for n in delta):
                        return tuple(n/(now-previous[1]) for n in delta)
                return None
            except UnsupportedOid:
                self.network_previous = None
                self.network_mode = '32' if self.network_mode == '64' else 'unsupported'
        return None


# The server bounds this cache to its four configured targets on configuration changes.
profiles = {}
