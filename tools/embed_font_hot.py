"""Embed the frequently used Latin banks without changing their pixel encoding."""
from pathlib import Path
import re

def generate(root):
    lines = ['#pragma once', '#include <Arduino.h>', 'namespace FontHot {']
    for size in (12, 16, 20, 26):
        data = (root / f'data/fonts/latin-{size}.bin').read_bytes()
        lines.append(f'const uint8_t latin{size}[] PROGMEM={{')
        lines.extend(','.join(map(str, data[i:i+32])) + ',' for i in range(0, len(data), 32))
        lines.append('};')
    index=(root/'include/font_index.h').read_text(encoding='utf8')
    def array(name):
        return list(map(int,re.search(r'\b'+name+r'\[\].*?=\{(.*?)\};',index)[1].split(',')))
    groups=array('glyphGroups')
    for group,size in [(0,s) for s in (12,16,20,26)]+[(1,12),(1,16),(1,20)]:
        name='latin' if group==0 else 'interface'
        suffix=str(size) if group==0 else f'Interface{size}'
        bank=(root/f'data/fonts/{name}-{size}.bin').read_bytes()
        starts,lengths,widths=array(f'offsets{size}'),array(f'lengths{size}'),array(f'advances{size}')
        offsets=[0]*len(groups); packed=bytearray()
        for glyph,glyph_group in enumerate(groups):
            if glyph_group!=group: continue
            offsets[glyph]=len(packed)
            payload=bank[24+starts[glyph]:24+starts[glyph]+lengths[glyph]]
            pixels=[];pos=0
            while pos<len(payload):
                token=payload[pos];pos+=1;count=(token&127)+1
                if token&128:
                    pixels.extend((payload[pos+j//2]>>((j%2)*4))&15 for j in range(count));pos+=(count+1)//2
                else: pixels.extend([0]*count)
            assert len(pixels)==size*size
            tight=[pixels[y*size+x] for y in range(size) for x in range(widths[glyph])]
            packed.extend(tight[j] | ((tight[j+1] if j+1<len(tight) else 0)<<4) for j in range(0,len(tight),2))
        lines.append(f'const uint32_t pixelsOffset{suffix}[] PROGMEM={{'+','.join(map(str,offsets))+'};')
        lines.append(f'const uint8_t pixels{suffix}[] PROGMEM={{')
        lines.extend(','.join(map(str,packed[i:i+32]))+',' for i in range(0,len(packed),32))
        lines.append('};')
    lines.append('}')
    (root / 'include/font_hot.h').write_text('\n'.join(lines)+'\n', encoding='utf8')

if __name__ == '__main__':
    generate(Path(__file__).resolve().parents[1])
