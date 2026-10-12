#pragma once
#include <Arduino.h>
#include "font_index.h"
#include "font_resource.h"
namespace Cjk { int clipTop=0,clipBottom=240,clipLeft=0,clipRight=240;uint16_t background=0x220a;
inline uint32_t next(const String& t,unsigned& p){unsigned c=(uint8_t)t[p++];if(c<128)return c;unsigned n=c>=240?3:c>=224?2:1;c&=(1u<<(6-n))-1;while(n--&&p<t.length())c=(c<<6)|((uint8_t)t[p++]&63);return c;}
inline int find(const uint16_t* table,int count,unsigned code){int l=0,r=count-1;while(l<=r){int m=(l+r)/2;unsigned c=pgm_read_word(table+m);if(c==code)return m;if(c<code)l=m+1;else r=m-1;}return -1;}
inline int lookup(unsigned c){return find(codes,sizeof(codes)/2,c);}
inline int lookup20(unsigned c){return find(codes20,sizeof(codes20)/2,c);}
inline bool hasChinese(const String& t){for(unsigned i=0;i<t.length();i++)if((uint8_t)t[i]>=128)return true;return false;}
inline int sizeOf(int font){return font==4?26:font==3?20:font==1?12:16;}
inline int advance(unsigned code,int font){int size=sizeOf(font);if(code>=128)return size;int i=font>=3?lookup20(code):lookup(code);if(i<0)return size/2;const uint8_t* a=font==1?advances12:font==3?advances20:font==4?advances26:advances16;return pgm_read_byte(a+i);}
inline int width(const String& text,int font){int w=0;unsigned p=0;while(p<text.length())w+=advance(next(text,p),font);return w;}
inline uint16_t blend(uint16_t f,uint16_t b,int a){return ((((f>>11)*a+(b>>11)*(15-a))/15)<<11)|(((((f>>5)&63)*a+((b>>5)&63)*(15-a))/15)<<5)|(((f&31)*a+(b&31)*(15-a))/15);}
// Decode one native glyph once; pixel lookups never rescan its RLE stream.
inline int coverage(int i,int x,int y,int font){return FontResource::coverage(i,x,y,font);}
template<class T> void draw(T& s,const String& text,int x,int y,int font,uint16_t color,int datum) {
  // Reject off-strip labels before UTF-8 width scans and font checks.
  const int size=sizeOf(font);
  const int top=datum==MC_DATUM ? y-size/2 : y;
  if(top>=clipBottom || top+size<=clipTop) return;
  if(!FontResource::available()) {
    s.setTextColor(color); s.setTextDatum(datum);
    s.drawString(hasChinese(text)?String("Font resource missing"):text,x,y,2); return;
  }
  const int total=datum==TR_DATUM || datum==MC_DATUM ? width(text,font) : 0;
  if(datum==TR_DATUM) x-=total;
  else if(datum==MC_DATUM) { x-=total/2; y-=size/2; }
  if(y>=clipBottom || y+size<=clipTop) return;
  const int first=clipTop>y?clipTop-y:0,last=clipBottom-y<size?clipBottom-y:size;
  // Each label needs sixteen colors, not three channel divisions per pixel.
  uint16_t shades[16];
  for(int a=1;a<16;a++) shades[a]=blend(color,background,a);
  unsigned p=0;
  while(p<text.length()) {
    unsigned code=next(text,p);
    int source=font,i=(font==3||font==4)?lookup20(code):lookup(code),adv=advance(code,font);
    if(x>=clipRight || x+adv<=clipLeft) { x+=adv; continue; }
    if((font==3||font==4)&&i<0) { i=lookup(code); source=2; }
    if(i<0) { i=lookup(0x25a1); source=2; }
    if(!FontResource::glyphAvailable(i,source)) {
      s.setTextColor(color);s.setTextDatum(TL_DATUM);s.drawString("?",x,y,2);x+=adv;continue;
    }
    int src=sizeOf(source),shift=(size-src)/2;
    int left=x<clipLeft?clipLeft-x:0,right=x+adv>clipRight?clipRight-x:adv;
    for(int yy=first;yy<last;yy++) for(int xx=left;xx<right;xx++) {
      int bx=xx-shift,by=yy-shift;
      if(bx<0||bx>=src||by<0||by>=src) continue;
      int a=coverage(i,bx,by,source);
      if(a) {
#ifdef ESP8266
        s.drawPixel(x+xx,y+yy,shades[a]);
#else
        s.fillRect(x+xx,y+yy,1,1,shades[a]);
#endif
      }
    }
    x+=adv;
  }
  s.setTextDatum(TL_DATUM);
}
}
