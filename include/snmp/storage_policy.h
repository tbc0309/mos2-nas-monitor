#pragma once
#include <cstring>
#include <initializer_list>
#include <stdint.h>
namespace StoragePolicy {
inline bool digits(const char *&name) {
  const char *begin=name;while(*name>='0'&&*name<='9')++name;
  return name!=begin;
}
inline bool wholeDisk(const char *name) {
  // Match whole Linux disks exactly, excluding partitions and eMMC boot areas.
  if(std::strncmp(name,"nvme",4)==0) {name+=4;return digits(name)&&*name++=='n'&&digits(name)&&!*name;}
  for(const char *prefix:{"mmcblk","sata"}) if(std::strncmp(name,prefix,std::strlen(prefix))==0) {
    name+=std::strlen(prefix);return digits(name)&&!*name;
  }
  for(const char *prefix:{"xvd","sd","hd","vd"}) if(std::strncmp(name,prefix,std::strlen(prefix))==0) {
    name+=std::strlen(prefix);const char *begin=name;while(*name>='a'&&*name<='z')++name;return name!=begin&&!*name;
  }
  return false;
}
inline bool numeric(uint8_t tag,bool valid,bool negative) {
  return valid && !negative && (tag==2 || tag==0x41 || tag==0x42 || tag==0x43 || tag==0x46);
}
inline bool diskValue(unsigned column,uint8_t tag,bool valid,bool negative,uint64_t value) {
  if (!numeric(tag,valid,negative)) return false;
  return column==6 ? value<=150 : (column==5 || column==13) && value>0 && value<=0x7fffffff;
}
inline bool capacity(uint64_t units,uint64_t size,uint64_t used) {
  return units && size && used<=size && size<=UINT64_MAX/units;
}
inline bool index(const char* value) {
  unsigned length=0;
  for(;*value;++value) if(*value<'0'||*value>'9'||++length>20) return false;
  return length>0;
}
inline bool needsVendorBootstrap(bool enabled,bool disksReady,bool volumesReady) {
  return enabled && !disksReady && volumesReady;
}
inline bool prefix(const char *value,const char *root) {return std::strncmp(value,root,std::strlen(root))==0;}
inline bool path(const char *value,const char *root) {
  const size_t length=std::strlen(root);
  return prefix(value,root)&&(value[length]==0||value[length]=='/');
}
inline bool synologyVolume(const char *name) {
  if(prefix(name,"/volumeUSB")) {
    name+=10;if(!digits(name))return false;
    if(!*name)return true;
    if(*name++!='/')return false;
    return prefix(name,"usbshare") && !std::strchr(name,'/');
  }
  if(!prefix(name,"/volume"))return false;
  name+=7;return digits(name)&&!*name;
}
inline bool candidate(const char *name,bool includeRoot,bool synology=false) {
  if (!name || !*name) return false;
  if(synology) return synologyVolume(name);
  // Storage type decides whether Windows/named volumes are disk-backed.
  if (*name!='/') return true;
  if (!includeRoot&&std::strcmp(name,"/")==0) return false;
  // TerraMaster mounts external USB volumes under /VolumeN/@usb/.
  const char *hidden=std::strstr(name,"/@");
  if ((hidden && !path(hidden,"/@usb")) || path(name,"/var/lib/docker") || path(name,"/var/lib/containers")) return false;
  for(const char *root:{"/dev","/proc","/sys","/boot","/rom","/snap"}) if(path(name,root))return false;
  if(path(name,"/run")&&!prefix(name,"/run/media/"))return false;
  if(path(name,"/tmp")&&!prefix(name,"/tmp/mnt/"))return false;
  return true;
}
inline bool diskBacked(const char *type) {
  return std::strcmp(type,"1.3.6.1.2.1.25.2.1.4")==0 ||
    std::strcmp(type,"1.3.6.1.2.1.25.2.1.5")==0 ||
    std::strcmp(type,"1.3.6.1.2.1.25.2.1.9")==0;
}
}
