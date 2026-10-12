#pragma once
#include "device_profile.h"
namespace IdentityLabel {
template<class Text> Text safeHost(const Text &name) {
  if(!name.length() || name.length()>64) return Text();
  for(unsigned i=0;i<name.length();++i) {
    const char c=name[i];
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.')) return Text();
  }
  return name;
}
// Net-SNMP uname descriptions begin with OS, hostname, kernel release.
// Arbitrary vendor prose is not a hostname; never guess from its second word.
template<class Text> Text descriptionHost(const Text &description) {
  for(unsigned i=0;i<description.length();++i) {
    const unsigned char c=description[i];
    if(c<32 || c>126) return Text();
  }
  const char *p=description.c_str();
  const char *systems[]={"Linux ","FreeBSD ","OpenBSD ","NetBSD ","Darwin ","SunOS "};
  bool matched=false;
  for(const char *system:systems) if(std::strncmp(p,system,std::strlen(system))==0) {p+=std::strlen(system);matched=true;break;}
  if(!matched) return Text();
  char host[65];unsigned length=0;
  while(*p && *p!=' ' && length<64) {
    const char c=*p++;
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.')) return Text();
    host[length++]=c;
  }
  if(!length || *p!=' ') return Text();
  host[length]=0;return Text(host);
}
inline const char *systemType(DeviceProfile::System system) {
  using DeviceProfile::System;
  switch(system) {
    case System::Synology:return "Synology";
    case System::Ubuntu:return "Ubuntu";
    case System::OpenWrt:return "OpenWrt";
    case System::Fnos:return "fnOS";
    case System::TerraMaster:return "TOS";
    case System::Qnap:return "QNAP";
    case System::Linux:return "Linux";
    default:return "SNMP";
  }
}
// Keep host identity separate from vendor model. User overrides bypass this label.
// A configurable hostname is never used to infer proprietary collection capabilities.
template<class Text>
Text automatic(const Text &host, const Text &model, bool verifiedModel, DeviceProfile::Match profile) {
  Text name=host.length()?host:Text("SNMP");
  Text kind=verifiedModel && model.length()?model:Text(systemType(profile.system));
  return name+Text(" (")+kind+Text(")");
}
}
