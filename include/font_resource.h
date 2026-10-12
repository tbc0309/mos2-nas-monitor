#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "font_index.h"
#if defined(ESP8266) || defined(MOS2_EMBED_LATIN)
#include "font_hot.h"
#endif
#ifdef ESP8266
#include <LittleFS.h>
#else
#include <fstream>
#include <cstdlib>
#include <string>
#endif

// The resource pack is independent of user configuration and is retained by factory reset.
namespace FontResource {
class Source {
  const uint8_t* flashData=nullptr;
  uint32_t flashBytes=0;
#ifdef ESP8266
  File file;
#else
  std::ifstream file;
#endif
public:
  bool open(const char* name) {
    close();
#if defined(ESP8266) || defined(MOS2_EMBED_LATIN)
    // The hot path bypasses filesystem seek/open while keeping identical RLE data.
    const uint8_t* banks[]={FontHot::latin12,FontHot::latin16,FontHot::latin20,FontHot::latin26};
    const uint32_t sizes[]={sizeof(FontHot::latin12),sizeof(FontHot::latin16),sizeof(FontHot::latin20),sizeof(FontHot::latin26)};
    const char* names[]={"latin-12.bin","latin-16.bin","latin-20.bin","latin-26.bin"};
    for(unsigned i=0;i<4;i++) if(!strcmp(name,names[i])) {
      flashData=banks[i]; flashBytes=sizes[i]; return true;
    }
#endif
#ifdef ESP8266
    char path[64]; snprintf(path,sizeof(path),"/fonts/%s",name);
    file = LittleFS.open(path, "r");
    return bool(file);
#else
    const char* overridePath = std::getenv("MOS2_FONT_DIRECTORY");
    std::string path = std::string(overridePath ? overridePath : "data/fonts") + "/" + name;
    file.open(path, std::ios::binary);
    if (!file && !overridePath) {
      file.clear();
      std::string here = __FILE__;
      path = here.substr(0, here.find_last_of("/\\")) + "/../data/fonts/" + std::string(name);
      file.open(path, std::ios::binary);
    }
    return bool(file);
#endif
  }
  void close() {
    flashData=nullptr; flashBytes=0;
#ifdef ESP8266
    file.close();
#else
    if (file.is_open()) file.close();
    file.clear();
#endif
  }
  uint32_t size() {
    if(flashData) return flashBytes;
#ifdef ESP8266
    return file.size();
#else
    file.seekg(0, std::ios::end); return uint32_t(file.tellg());
#endif
  }
  bool read(uint32_t offset, uint8_t* bytes, size_t count) {
    if(flashData) {
      if(offset>flashBytes || count>flashBytes-offset) return false;
      for(size_t i=0;i<count;i++) bytes[i]=pgm_read_byte(flashData+offset+i);
      return true;
    }
#ifdef ESP8266
    if (!file.seek(offset, SeekSet)) return false;
    const int received=file.read(bytes,count);
    return received>=0 && size_t(received)==count;
#else
    file.clear(); file.seekg(offset); file.read(reinterpret_cast<char*>(bytes), count);
    return size_t(file.gcount()) == count;
#endif
  }
};
static Source source;
static bool attempted = false, ready = false;
static uint32_t reads = 0, cacheHits = 0, readErrors = 0, tick = 0;
struct Entry {
  uint16_t index=0,offset=0;
  uint8_t font=0,width=0;
  bool valid=false;
  uint32_t stamp=0;
};
// Narrow Latin glyphs no longer reserve a full 26x26 square. The pool and its
// twelve descriptors use the same 1392-byte budget as the old four fixed slots.
static constexpr unsigned CacheEntries=12, PixelPoolBytes=1248;
static Entry cache[CacheEntries];
static uint8_t pixelPool[PixelPoolBytes];
static unsigned poolUsed=0;
inline unsigned cacheBytes() { return sizeof(cache)+sizeof(pixelPool); }
inline unsigned entryBytes(const Entry& e) {
  unsigned height=e.font==4 ? 26 : e.font==3 ? 20 : e.font==1 ? 12 : 16;
  return (e.width*height+1)/2;
}
inline void evict(unsigned index) {
  Entry& e=cache[index]; if(!e.valid) return;
  unsigned bytes=entryBytes(e),end=e.offset+bytes;
  memmove(pixelPool+e.offset,pixelPool+end,poolUsed-end);
  for(auto& other:cache) if(other.valid && other.offset>e.offset) other.offset-=bytes;
  poolUsed-=bytes; e.valid=false;
}
static int lastEntry = -1;
inline uint32_t word(const uint8_t* p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
// One file handle and a bounded variable-width glyph pool serve all sixteen banks.
static int openedBank = -1;
static uint16_t checkedBanks = 0, failedBanks = 0;
static uint32_t bankOpens = 0, validationReads = 0;
inline void bankName(int bank, char* name, size_t length) {
  const char* group = bank%4==0 ? "latin" : bank%4==1 ? "interface" : bank%4==2 ? "weather" : "geography";
  const int sizes[] = {12,16,20,26};
  snprintf(name,length,"%s-%d.bin",group,sizes[bank/4]);
}
inline bool begin() {
  source.close(); attempted=true; ready=false; lastEntry=-1; openedBank=-1;
  checkedBanks=failedBanks=0; poolUsed=0;
  for (auto& e:cache) e.valid=false;
  if (!source.open("catalog.bin") || source.size()!=24) { source.close(); return false; }
  uint8_t header[24];
  bool ok=source.read(0,header,sizeof(header)) && !memcmp(header,"MOSFSET3",8) &&
    word(header+8)==Cjk::resourceBytes && word(header+12)==Cjk::resourceCrc &&
    word(header+16)==16 && word(header+20)==16;
  source.close(); ready=ok; return ok;
}
inline bool available() { return attempted ? ready : begin(); }
inline bool ensureBank(int bank) {
  if (bank<0 || bank>=16 || !available() || (failedBanks & (1u<<bank))) return false;
  if (openedBank==bank) return true;
  char name[40]; bankName(bank,name,sizeof(name));
  openedBank=-1;
  const uint32_t expected=pgm_read_dword(Cjk::bankBytes+bank);
  const uint32_t expectedCrc=pgm_read_dword(Cjk::bankCrc+bank);
  bool ok=source.open(name) && source.size()==expected;
  uint8_t header[24];
  ok=ok && source.read(0,header,sizeof(header)) && !memcmp(header,"MOSFBK03",8) &&
    word(header+8)==expected && word(header+12)==expectedCrc &&
    word(header+16)==uint32_t(bank) && word(header+20)==16;
  if (ok && !(checkedBanks & (1u<<bank))) {
    uint8_t buffer[512]; uint32_t crc=0xffffffff;
    for (uint32_t offset=24; offset<expected && ok;) {
      size_t count=expected-offset; if (count>sizeof(buffer)) count=sizeof(buffer);
      ok=source.read(offset,buffer,count); validationReads++;
      if (!ok) break;
      for (size_t i=0;i<count;i++) {
        crc^=buffer[i];
        for (int bit=0;bit<8;bit++) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
      }
      offset+=count;
#ifdef ESP8266
      yield();
#endif
    }
    ok=ok && (crc^0xffffffffu)==expectedCrc;
  }
  if (!ok) { source.close(); failedBanks|=1u<<bank; readErrors++; return false; }
  checkedBanks|=1u<<bank; openedBank=bank; bankOpens++; return true;
}
inline bool glyphAvailable(int i,int font) {
  if (i<0 || i>=int(sizeof(Cjk::codes)/sizeof(Cjk::codes[0])) || font<1 || font>4) return false;
  int bank=(font-1)*4+pgm_read_byte(Cjk::glyphGroups+i);
  if (!available() || (failedBanks & (1u<<bank))) return false;
  return (checkedBanks & (1u<<bank)) || ensureBank(bank);
}
inline void put(Entry& e,unsigned position,uint8_t alpha) {
  unsigned size=e.font==4 ? 26 : e.font==3 ? 20 : e.font==1 ? 12 : 16;
  unsigned x=position%size,y=position/size;
  if(x>=e.width) return;
  unsigned pixel=y*e.width+x;
  pixelPool[e.offset+pixel/2]|=alpha<<((pixel%2)*4);
}
// Stream compressed bytes in bounded chunks rather than reserving a 1 KiB stack array.
class GlyphReader {
  uint32_t offset, remaining;
  unsigned position=0, buffered=0;
  uint8_t bytes[128];
public:
  GlyphReader(uint32_t start,uint32_t count):offset(start),remaining(count) {}
  bool next(uint8_t& value) {
    if (position==buffered) {
      if (!remaining) return false;
      buffered=remaining>sizeof(bytes) ? sizeof(bytes) : remaining; position=0;
      if (!source.read(offset,bytes,buffered)) return false;
      offset+=buffered; remaining-=buffered;
    }
    value=bytes[position++]; return true;
  }
  bool finished() const { return !remaining && position==buffered; }
};
inline bool decode(Entry& entry,int i,int font) {
  if (!glyphAvailable(i,font)) return false;
  if (!ensureBank((font-1)*4+pgm_read_byte(Cjk::glyphGroups+i))) return false;
  int size=font==4 ? 26 : font==3 ? 20 : font==1 ? 12 : 16;
  const uint32_t* offsets=font==1 ? Cjk::offsets12 : font==2 ? Cjk::offsets16 : font==3 ? Cjk::offsets20 : Cjk::offsets26;
  const uint16_t* lengths=font==1 ? Cjk::lengths12 : font==2 ? Cjk::lengths16 : font==3 ? Cjk::lengths20 : Cjk::lengths26;
  uint32_t first=pgm_read_dword(offsets+i),length=pgm_read_word(lengths+i);
  if (length>1024 || first>0xffffffffu-24-length || 24+first+length>pgm_read_dword(Cjk::bankBytes+openedBank)) return false;
  memset(pixelPool+entry.offset,0,entryBytes(entry));
  GlyphReader reader(24+first,length); unsigned pos=0;
  while (!reader.finished()) {
    uint8_t token; if (!reader.next(token)) return false;
    unsigned count=(token&127)+1;
    if (pos+count>unsigned(size*size)) return false;
    if (token&128) {
      for (unsigned j=0;j<count;j+=2) {
        uint8_t pair; if (!reader.next(pair)) return false;
        put(entry,pos++,pair&15);
        if (j+1<count) put(entry,pos++,pair>>4);
      }
    } else pos+=count;
  }
  return pos==unsigned(size*size);
}
inline int coverage(int i, int x, int y, int font) {
  if (i<0 || i>=int(sizeof(Cjk::codes)/sizeof(Cjk::codes[0])) || font<1 || font>4 || !available()) return 0;
  int size = font == 4 ? 26 : font == 3 ? 20 : font == 1 ? 12 : 16;
  if (x < 0 || y < 0 || x >= size || y >= size) return 0;
#if defined(ESP8266) || defined(MOS2_EMBED_LATIN)
  unsigned group=pgm_read_byte(Cjk::glyphGroups+i);
  if(group==0 || (group==1 && font<=3)) {
    if(!glyphAvailable(i,font)) return 0;
    const uint8_t* widths=font==1?Cjk::advances12:font==2?Cjk::advances16:font==3?Cjk::advances20:Cjk::advances26;
    unsigned width=pgm_read_byte(widths+i);
    if(unsigned(x)>=width) return 0;
    const uint32_t* offsets=group==1 ? (font==1?FontHot::pixelsOffsetInterface12:font==2?FontHot::pixelsOffsetInterface16:FontHot::pixelsOffsetInterface20) : font==1?FontHot::pixelsOffset12:font==2?FontHot::pixelsOffset16:font==3?FontHot::pixelsOffset20:FontHot::pixelsOffset26;
    const uint8_t* data=group==1 ? (font==1?FontHot::pixelsInterface12:font==2?FontHot::pixelsInterface16:FontHot::pixelsInterface20) : font==1?FontHot::pixels12:font==2?FontHot::pixels16:font==3?FontHot::pixels20:FontHot::pixels26;
    unsigned pixel=y*width+x;
    return (pgm_read_byte(data+pgm_read_dword(offsets+i)+pixel/2)>>((pixel%2)*4))&15;
  }
#endif
  int selected = lastEntry;
  if (selected < 0 || !cache[selected].valid || cache[selected].index != i || cache[selected].font != font) {
    if (!glyphAvailable(i,font)) return 0;
    selected = -1;
    for (int j = 0; j < int(CacheEntries); j++) if (cache[j].valid && cache[j].index == i && cache[j].font == font) { selected = j; break; }
    if (selected >= 0) cacheHits++;
    else {
      selected = 0;
      for (int j = 0; j < int(CacheEntries); j++) {
        if (!cache[j].valid) { selected = j; break; }
        if (cache[j].stamp < cache[selected].stamp) selected = j;
      }
      evict(selected);
      const uint8_t* widths=font==1 ? Cjk::advances12 : font==2 ? Cjk::advances16 : font==3 ? Cjk::advances20 : Cjk::advances26;
      unsigned width=pgm_read_byte(widths+i), bytes=(width*size+1)/2;
      if(!width || width>unsigned(size) || bytes>PixelPoolBytes) { readErrors++; return 0; }
      while(poolUsed+bytes>PixelPoolBytes) {
        int oldest=-1;
        for(int j=0;j<int(CacheEntries);j++) if(cache[j].valid && (oldest<0 || cache[j].stamp<cache[oldest].stamp)) oldest=j;
        if(oldest<0) { readErrors++; return 0; }
        evict(oldest);
      }
      Entry& e=cache[selected]; e.font=font; e.width=width; e.offset=poolUsed;
      if(!decode(e,i,font)) {
        readErrors++; failedBanks|=1u<<((font-1)*4+pgm_read_byte(Cjk::glyphGroups+i));
        source.close(); openedBank=-1; return 0;
      }
      poolUsed+=bytes;
      reads++; e.index = i; e.font = font; e.valid = true;
    }
    lastEntry = selected;
    cache[selected].stamp = ++tick;
  }
  if (failedBanks && !glyphAvailable(i,font)) return 0;
  const Entry& e=cache[selected];
  if(x>=e.width) return 0;
  unsigned p=y*e.width+x;
  return (pixelPool[e.offset+p/2]>>((p%2)*4))&15;
}
}
