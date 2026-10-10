#pragma once
class IPAddress{public:bool operator!=(const IPAddress&)const{return false;}};
class WiFiUDP{public:bool begin(int){return true;}void stop(){}void beginPacket(IPAddress,unsigned){}void write(const unsigned char*,unsigned){}bool endPacket(){return true;}int parsePacket(){return 0;}int available(){return 0;}int read(){return 0;}int read(unsigned char*,unsigned){return 0;}IPAddress remoteIP(){return {};}};
