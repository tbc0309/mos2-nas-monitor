#pragma once
#include <vector>
#include <cstring>
static std::vector<unsigned char> testUdpPacket;
class IPAddress{public:bool operator!=(const IPAddress&)const{return false;}};
class WiFiUDP{public:bool begin(int){return true;}void stop(){}void beginPacket(IPAddress,unsigned){}void write(const unsigned char*,unsigned){}bool endPacket(){return true;}int parsePacket(){return testUdpPacket.size();}int available(){return testUdpPacket.size();}int read(){if(testUdpPacket.empty())return -1;int v=testUdpPacket.front();testUdpPacket.erase(testUdpPacket.begin());return v;}int read(unsigned char* p,unsigned n){unsigned len=std::min(n,unsigned(testUdpPacket.size()));std::memcpy(p,testUdpPacket.data(),len);testUdpPacket.erase(testUdpPacket.begin(),testUdpPacket.begin()+len);return len;}IPAddress remoteIP(){return {};}};
