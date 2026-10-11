#pragma once
#include <string>
#include <cstdint>
#include <cstdlib>
class String:public std::string {
 public:String()=default;using std::string::string;String(const std::string& s):std::string(s){}String(unsigned n):std::string(std::to_string(n)){}String(int n):std::string(std::to_string(n)){}
 int indexOf(char c,unsigned p=0)const{auto i=find(c,p);return i==npos?-1:int(i);}String substring(unsigned a,unsigned b)const{return substr(a,b-a);}long toInt()const{return std::strtol(c_str(),nullptr,10);}
};
static uint32_t testMillis = 0;
inline uint32_t millis(){return testMillis;}
