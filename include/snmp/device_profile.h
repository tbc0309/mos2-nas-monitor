#pragma once
#include <stdint.h>
#include <cstring>
#include <cctype>
namespace DeviceProfile {
enum class System : uint8_t { Unknown, Generic, Linux, Ubuntu, OpenWrt, Fnos, Synology, Qnap, TerraMaster };
enum class Evidence : uint8_t { None, Description, Enterprise, VendorResponse };
struct Match { System system = System::Unknown; Evidence evidence = Evidence::None; };
inline bool contains(const char *text, const char *token) {
  if (!text || !token || !*token) return false;
  for (; *text; ++text) {
    const char *a=text, *b=token;
    while (*a && *b && std::tolower(static_cast<unsigned char>(*a)) == std::tolower(static_cast<unsigned char>(*b))) { ++a; ++b; }
    if (!*b) return true;
  }
  return false;
}
inline bool subtree(const char *oid, const char *root) {
  if (!oid || !root) return false;
  const size_t length=std::strlen(root);
  return std::strncmp(oid,root,length)==0 && (oid[length]==0 || oid[length]=='.');
}
inline Match classify(const char *description, const char *objectId, bool verifiedSynologyModel=false) {
  // sysName and user labels are intentionally excluded: hostnames are not OS evidence.
  if (verifiedSynologyModel) return {System::Synology,Evidence::VendorResponse};
  if (subtree(objectId,"1.3.6.1.4.1.6574")) return {System::Synology,Evidence::Enterprise};
  if (subtree(objectId,"1.3.6.1.4.1.24681") || subtree(objectId,"1.3.6.1.4.1.55062")) return {System::Qnap,Evidence::Enterprise};
  const struct { const char *token; System system; } rules[] = {
    {"synology",System::Synology},{"openwrt",System::OpenWrt},{"ubuntu",System::Ubuntu},
    {"fnos",System::Fnos},{"terramaster",System::TerraMaster},{"qnap",System::Qnap},{"linux",System::Linux}};
  for (const auto &rule:rules) if (contains(description,rule.token)) return {rule.system,Evidence::Description};
  return {(description && *description) || (objectId && *objectId) ? System::Generic : System::Unknown,Evidence::None};
}
inline const char *name(System system) {
  switch(system) {
    case System::Linux:return "linux"; case System::Ubuntu:return "ubuntu"; case System::OpenWrt:return "openwrt";
    case System::Fnos:return "fnos"; case System::Synology:return "synology"; case System::Qnap:return "qnap";
    case System::TerraMaster:return "terramaster"; case System::Generic:return "generic"; default:return "unknown";
  }
}
inline bool synology(System system) { return system==System::Synology; }
}
