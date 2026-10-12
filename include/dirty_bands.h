#pragma once
#include "dashboard.h"

// Fixed-size fingerprints avoid retaining a second State or copied text strings.
// Ten-row bands repaint their complete RGB565 contents, including old text extents.
namespace DirtyBands {
constexpr unsigned Height=10, Count=24;
struct Plan {
  uint32_t hash[Count];
  Plan() { for(auto& v:hash) v=2166136261u; }
  static uint32_t fingerprint(const String& value) {
    uint32_t h=2166136261u;
    for(unsigned i=0;i<value.length();i++) h=(h^uint8_t(value[i]))*16777619u;
    return h;
  }
  void addHash(int top,int bottom,uint32_t h) {
    for(unsigned b=0;b<Count;b++) if(int((b+1)*Height)>top && int(b*Height)<bottom)
      hash[b]=(hash[b]^h)*16777619u;
  }
  void add(int top,int bottom,const String& value) { addHash(top,bottom,fingerprint(value)); }
  template<class T> void number(int top,int bottom,T value) {
    uint32_t h=2166136261u;const uint8_t* p=reinterpret_cast<const uint8_t*>(&value);
    for(unsigned i=0;i<sizeof(value);i++) h=(h^p[i])*16777619u;
    addHash(top,bottom,h);
  }
  // Match both rounded text and the integer bar length, without retaining strings.
  void percentage(int top,int bottom,float value,int width,bool integerText=false) {
    add(top,bottom,value<0?String("--"):String(value,integerText?0:1));
    number(top,bottom,value<0?-1:constrain(int(value*width/100),0,width));
  }

};
inline Plan monitor(const Dashboard::State& d,unsigned page) {
  Plan p;
  p.number(0,30,d.online);
  if(page==0) {
    p.add(0,33,Dashboard::overviewRateLabel(d.rx));p.add(0,33,Dashboard::overviewRateLabel(d.tx));
    p.percentage(120,159,d.cpu,123);p.percentage(160,198,d.memory,123,true);p.number(120,210,d.temp);
    p.add(198,214,d.nas);p.number(198,214,d.privacy);p.number(198,214,d.online);
  } else if(page==1) {
    p.add(37,70,Dashboard::rateLabel(d.rx));p.add(72,106,Dashboard::rateLabel(d.tx));p.add(192,211,d.iface);
  } else if(page==2) {
    p.percentage(35,82,d.cpu,220);p.percentage(85,132,d.memory,220);
    p.number(136,185,d.memoryTotal);p.number(136,185,d.memoryAvail);p.number(136,185,d.memory<0);
  } else if(page==4) {
    p.add(36,66,d.model);p.add(73,89,d.nas);p.number(96,112,d.uptime/3600);
    p.add(119,135,d.device);p.number(142,158,d.rssi);
    p.add(171,197,d.source);p.add(171,197,d.security);p.number(171,197,d.online);p.number(171,197,d.online?0:d.age);
  } else if(page==8) {
    p.number(36,213,d.diskCount);p.number(36,213,d.diskGroup);p.add(36,213,d.model);
    unsigned start=d.diskCount>4?(d.diskGroup%((d.diskCount+3)/4))*4:0;
    for(unsigned i=start;i<d.diskCount && i<start+4;i++) {
      const auto& disk=d.disks[i];int y=36+(i-start)*42;
      p.add(y,y+39,disk.name);p.add(y,y+39,disk.model);p.number(y,y+39,disk.temp);
      p.number(y,y+39,disk.status);p.number(y,y+39,disk.health);
    }
  } else if(page==9) {
    p.number(36,213,d.volumeCount);p.number(36,213,d.volumeGroup);
    unsigned start=d.volumeCount>3?(d.volumeGroup%((d.volumeCount+2)/3))*3:0;
    for(unsigned i=start;i<d.volumeCount && i<start+3;i++) {
      const auto& v=d.volumes[i];int y=36+(i-start)*55;
      p.add(y,y+52,v.name);p.number(y,y+52,v.valid);p.number(y,y+52,v.used);
      p.number(y,y+52,v.size);p.number(y,y+52,v.units);
    }
  }
  if(page==0 || page==1) {
    int top=page==0?38:112,bottom=page==0?112:182;p.number(top,bottom,d.historyCount);
    unsigned start=d.historyCount>10?d.historyCount-10:0;
    for(unsigned i=start;i<d.historyCount;i++) { p.number(top,bottom,d.rxHistory[i]);p.number(top,bottom,d.txHistory[i]); }
  }
  return p;
}
inline uint32_t difference(const Plan& previous,const Plan& current) {
  uint32_t mask=0;
  for(unsigned i=0;i<Count;i++) if(previous.hash[i]!=current.hash[i]) mask|=1u<<i;
  return mask;
}
inline bool intersects(uint32_t mask,unsigned top,unsigned rows) {
  for(unsigned i=top/Height;i<Count && i*Height<top+rows;i++) if(mask&(1u<<i)) return true;
  return false;
}
constexpr uint32_t All=(1u<<Count)-1;
}
