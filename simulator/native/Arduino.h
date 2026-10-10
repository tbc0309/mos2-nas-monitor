#pragma once
#include <string>
#include <cstdint>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <iomanip>
class String:public std::string {
 public:String()=default;using std::string::string;String(const std::string& s):std::string(s){}String(unsigned n):std::string(std::to_string(n)){}String(int n):std::string(std::to_string(n)){}
 String(double n,int decimals){std::ostringstream s;s<<std::fixed<<std::setprecision(decimals)<<n;assign(s.str());}
 bool startsWith(const char* v)const{return find(v)==0;}
 unsigned length()const{return size();}
 int indexOf(char c,unsigned p=0)const{auto i=find(c,p);return i==npos?-1:int(i);}String substring(unsigned a,unsigned b)const{return substr(a,b-a);}long toInt()const{return std::strtol(c_str(),nullptr,10);}
};
inline uint32_t millis(){return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
template<class T>T constrain(T v,T low,T high){return v<low?low:v>high?high:v;}
#define TL_DATUM 0
#define TR_DATUM 2
#define MC_DATUM 4

#define PROGMEM
#define pgm_read_dword(p) (*(p))

#define pgm_read_word(p) (*(p))

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
