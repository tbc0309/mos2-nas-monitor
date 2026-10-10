#pragma once
#include <string>
#include <cstdint>
#include <cstdlib>
class String:public std::string {
public:
 using std::string::string;String()=default;String(const std::string& s):std::string(s){}
 unsigned length()const{return size();}
 int indexOf(const char* s,unsigned start=0)const{auto n=find(s,start);return n==npos?-1:int(n);}
 int indexOf(char c,unsigned start=0)const{auto n=find(c,start);return n==npos?-1:int(n);}
 String substring(unsigned a,unsigned b=std::string::npos)const{return substr(a,b==std::string::npos?b:b-a);}
 void replace(const char* old,const char* next){size_t p=0;while((p=find(old,p))!=npos){std::string::replace(p,std::char_traits<char>::length(old),next);p+=std::char_traits<char>::length(next);}}
};
inline uint32_t millis(){return 1234;}
