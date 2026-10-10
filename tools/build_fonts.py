from pathlib import Path
import re,math,os
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];p=root/'include/cjk_font.h';old=p.read_text(encoding='utf-8-sig');codes=list(map(int,re.search(r'codes\[\d+\] PROGMEM=\{(.*?)\}',old,re.S)[1].split(',')))
latinpath=os.environ.get('FONT_PATH','TencentSans-W7.ttf')
if not Path(latinpath).is_file():raise SystemExit('Set FONT_PATH to your licensed TencentSans W7 font file.')

import freetype
from functools import lru_cache
FLAGS=freetype.FT_LOAD_RENDER|freetype.FT_LOAD_TARGET_MONO|freetype.FT_LOAD_FORCE_AUTOHINT
@lru_cache(None)
def prepared(points,cell,script):
 # A shared baseline for each size, including descenders and negative bearings.
 for pixels in range(points,6,-1):
  face=freetype.Face(latinpath);face.set_pixel_sizes(0,pixels)
  ascent=descent=0;wide=0
  for code in (range(32,127) if script=="latin" else [c for c in (codes if cell<=16 else codes20) if c>=128]):
   face.load_char(chr(code),FLAGS);g=face.glyph;b=g.bitmap
   if b.rows:
    ascent=max(ascent,g.bitmap_top);descent=max(descent,b.rows-g.bitmap_top)
    wide=max(wide,b.width)
  if ascent+descent<=cell and wide<=cell:
   print('Grid-fit cell',cell,'pixels',pixels,'baseline',ascent)
   return face,ascent
 raise ValueError('No fitting font size')
def raster(font,code,width,height,baseline):
 face,baseline=prepared(font.size,height,"latin" if code<128 else "cjk");face.load_char(chr(code),FLAGS);g=face.glyph;b=g.bitmap
 im=Image.new('L',(width,height));buf=b.buffer;left=max(0,g.bitmap_left);top=baseline-g.bitmap_top
 if left+b.width>width:left=width-b.width
 assert left>=0 and top>=0 and top+b.rows<=height,(code,width,height,b.width,b.rows,left,top)
 for y in range(b.rows):
  for x in range(b.width):
   if buf[y*abs(b.pitch)+x//8]&(128>>(x%8)):im.putpixel((left+x,top+y),255)
 return im
def extent(font,code):
 cell=12 if font.size==11 else 16 if font.size==14 else 20 if font.size==20 else 26
 face,_=prepared(font.size,cell,"latin");face.load_char(chr(code),FLAGS);g=face.glyph
 return max(2,math.ceil(g.advance.x/64),max(0,g.bitmap_left)+g.bitmap.width)
def latin(points):
 return ImageFont.truetype(latinpath,points)
# Tencent W7 native raster at every UI size; no scaled Chinese bitmap.
f12=latin(11);rows12=[]
for code in codes:
 im=raster(f12,code,12,12,10)
 rows12.append('{'+','.join(str(sum((1 if im.getpixel((x,y))>=112 else 0)<<(15-x) for x in range(12))) for y in range(12))+'}')
table='const uint16_t pixels12[][12] PROGMEM={'+','.join(rows12)+'};'
charset='周星期日一二三四五六月天津晴多云阴小中大暴雨雪雷阵夹雾霾沙尘风东南西北级优良轻重度温湿空气污染微转冰冻细颗粒未获取暂不可用载入天气同步时间贪睡分钟闹钟响铃停止开启关闭今日明最高最低北京上海广州深圳重庆成都长沙苏州南京杭州郑西安武汉沈阳大连济南青岛昆明石家庄保定唐山'
charset+=''.join(c for c in (root/'include/locales/lcd_zh.h').read_text(encoding='utf8') if 0x4e00<=ord(c)<=0x9fff)
codes20=sorted(set(range(32,127))|set(map(ord,charset)));s=['#pragma once','#include <Arduino.h>','namespace Cjk { int clipTop=0,clipBottom=240;uint16_t background=0x220a;',f'const uint16_t codes[{len(codes)}] PROGMEM={{'+','.join(map(str,codes))+'};',table]
fontpath=latinpath
for size,points,baseline,values in [(16,14,13,codes),(20,20,16,codes20)]:
 f=ImageFont.truetype(fontpath,points)
 try:f.set_variation_by_axes([550 if a['name']==b'Weight' else a['default'] for a in f.get_variation_axes()])
 except:pass
 if size==20:s.append('const uint16_t codes20[] PROGMEM={'+','.join(map(str,values))+'};')
 lf=latin(points)
 s.append(f'const uint8_t advances{size}[] PROGMEM={{'+','.join(str(extent(lf,c)) for c in range(32,127))+'};')
 s.append(f'const uint8_t pixels{size}[][{size*size//(2 if size==20 else 8)}] PROGMEM={{')
 for code in values:
  im=raster(lf if code<127 else f,code,size,size,baseline);step=2 if size==20 else 8;bits=4 if size==20 else 1;levels=15 if size==20 else 1
  if code>=128:im=im.point(lambda v:255 if v>=112 else 0)
  s.append('{'+','.join(hex(sum(round(im.getpixel((x+k,y))*levels/255)<<(bits*k) for k in range(step))) for y in range(size) for x in range(0,size,step))+'},')
 s.append('};')
# Native solid-pixel Tencent Latin subset on the same baseline as Chinese.
lf=latin(14);s.append('const uint8_t latin16[][64] PROGMEM={')
for code in range(32,127):
 im=raster(lf,code,16,16,13)
 s.append('{'+','.join(str(sum(round(im.getpixel((x+k,y))*3/255)<<(2*k) for k in range(4))) for y in range(16) for x in range(0,16,4))+'},')
s.append('};')
large=[c for c in codes20 if c>=128];s.append('const uint16_t codes26[] PROGMEM={'+','.join(map(str,large))+'};');s.append('const uint8_t chinese26[][104] PROGMEM={')
f=ImageFont.truetype(fontpath,24)
try:f.set_variation_by_axes([550 if a['name']==b'Weight' else a['default'] for a in f.get_variation_axes()])
except:pass
for code in large:
 im=raster(f,code,26,26,22)
 s.append('{'+','.join(str(sum((1 if im.getpixel((x+k,y))>=112 else 0)<<k for k in range(8) if x+k<26)) for y in range(26) for x in range(0,26,8))+'},')
s.append('};')
# Native 26px Latin glyphs, packed to their individual widths to fit ESP8266 flash.
lf=latin(26);widths=[];offsets=[0];data=[]
for code in range(32,127):
 width=extent(lf,code);widths.append(width)
 im=raster(lf,code,width,26,21)
 for y in range(26):
  for x in range(0,width,4):data.append(sum(round(im.getpixel((x+k,y))*3/255)<<(2*k) for k in range(4) if x+k<width))
 offsets.append(len(data))
s.extend(['const uint8_t advances26[] PROGMEM={'+','.join(map(str,widths))+'};','const uint16_t offsets26[] PROGMEM={'+','.join(map(str,offsets))+'};','const uint8_t pixels26[] PROGMEM={'+','.join(map(str,data))+'};'])
tail=old[old.index('inline uint32_t next'):old.index('inline int advance')];tail=re.sub(r'inline int lookup20[^\n]*\n','',tail);tail=re.sub(r'const uint8_t advances12[^\n]*\n','',tail);s.append(tail)
s.append('const uint8_t advances12[] PROGMEM={'+','.join(str(extent(latin(11),c)) for c in range(32,127))+'};')
s.append('''inline int lookup20(unsigned code){int lo=0,hi=sizeof(codes20)/2-1;while(lo<=hi){int mid=(lo+hi)/2;unsigned v=pgm_read_word(codes20+mid);if(v==code)return mid;if(v<code)lo=mid+1;else hi=mid-1;}return -1;}
inline int advance(unsigned code,int font){int size=font==4?26:font==3?20:font==1?12:16;if(code>=128)return size;if(font==1)return code>=32&&code<=126?pgm_read_byte(advances12+code-32):6;if(code<32||code>126)return size/2;if(font==4)return pgm_read_byte(advances26+code-32);return font==3?pgm_read_byte(advances20+code-32):pgm_read_byte(advances16+code-32)*(font==4?3:2)/2;}
inline int width(const String& text,int font){int w=0;unsigned p=0;while(p<text.length())w+=advance(next(text,p),font);return w;}
inline uint16_t blend(uint16_t f,uint16_t b,int a){return ((((f>>11)*a+(b>>11)*(15-a))/15)<<11)|(((((f>>5)&63)*a+((b>>5)&63)*(15-a))/15)<<5)|(((f&31)*a+(b&31)*(15-a))/15);}
inline int coverage(int i,int x,int y,int font){if(font==1)return pgm_read_word(&pixels12[i][y])&(1u<<(15-x))?15:0;if(font==3){int pos=y*20+x;return (pgm_read_byte(&pixels20[i][pos/2])>>((pos%2)*4))&15;}int pos=y*16+x;if(i<95)return ((pgm_read_byte(&latin16[i][pos/4])>>((pos%4)*2))&3)*5;return (pgm_read_byte(&pixels16[i][pos/8])>>(pos%8))&1?15:0;}
template<class T> void draw(T& s,const String& text,int x,int y,int font,uint16_t color,int datum){int size=font==4?26:font==3?20:font==1?12:16,total=width(text,font);if(datum==TR_DATUM)x-=total;else if(datum==MC_DATUM){x-=total/2;y-=size/2;}if(y>=clipBottom||y+size<=clipTop)return;int first=clipTop>y?clipTop-y:0,last=clipBottom-y<size?clipBottom-y:size;unsigned p=0;while(p<text.length()){unsigned code=next(text,p);int sourceFont=font,i=font==3?lookup20(code):lookup(code),adv=advance(code,font);if(font==3&&i<0){i=lookup(code);sourceFont=2;}if(font==4&&code>=32&&code<=126){int offset=pgm_read_word(offsets26+code-32),stride=(adv+3)/4;for(int yy=first;yy<last;yy++)for(int xx=0;xx<adv;xx++){int a=((pgm_read_byte(pixels26+offset+yy*stride+xx/4)>>((xx%4)*2))&3)*5;if(a)s.fillRect(x+xx,y+yy,1,1,blend(color,background,a));}x+=adv;continue;}if(font==4&&code>=128){int j=-1;for(unsigned k=0;k<sizeof(codes26)/2;k++)if(pgm_read_word(codes26+k)==code){j=k;break;}if(j>=0){for(int yy=first;yy<last;yy++)for(int xx=0;xx<26;xx++)if((pgm_read_byte(&chinese26[j][yy*4+xx/8])>>(xx%8))&1)s.fillRect(x+xx,y+yy,1,1,color);x+=adv;continue;}}int src=sourceFont==3?20:sourceFont==1?12:16;for(int yy=first;yy<last;yy++)for(int xx=0;xx<adv;xx++){if(i<0)continue;int bx=xx-(size-src)/2,by=yy-(size-src)/2;if(bx<0||bx>=src||by<0||by>=src)continue;int a=coverage(i,bx,by,sourceFont);if(a)s.fillRect(x+xx,y+yy,1,1,blend(color,background,a));}x+=adv;}s.setTextDatum(TL_DATUM);}
}
''');p.write_text('\n'.join(s),encoding='utf8');print('20px glyphs:',len(codes20))
