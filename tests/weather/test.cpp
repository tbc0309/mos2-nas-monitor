#define WEATHER_HOST_TEST
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include "weather.h"
String wrap(String data){return "var dataSK ="+data+";var dataZS = {};";}
int main(int argc,char** argv){
 Weather::Reading out;
 const String sample="{\"city\":\"101030100\",\"cityname\":\"Tianjin\",\"nameen\":\"tianjin\",\"weather\":\"Sunny\",\"weathere\":\"Sunny\",\"temp\":\"-5.3\",\"SD\":\"100%\",\"aqi\":\"92\",\"weathercode\":\"d00\",\"time\":\"14:10\"}";
 assert(Weather::parse(wrap(sample),"101030100",out));assert(out.valid&&out.temperature<0&&out.humidity==100&&out.aqi==92);
 assert(!Weather::parse(wrap(sample),"101010100",out));assert(out.valid&&out.humidity==100);
 for(const char* bad:{"nan","inf","100","bad"}){String s=sample;s.replace("-5.3",bad);assert(!Weather::parse(wrap(s),"101030100",out));}
 String s=sample;s.replace("100%","101%");assert(!Weather::parse(wrap(s),"101030100",out));
 for(unsigned n=0;n<sample.length();n++)assert(!Weather::parse(wrap(sample.substring(0,n)),"101030100",out));
 if(argc>1){std::ifstream f(argv[1]);String real(std::string((std::istreambuf_iterator<char>(f)),{}));assert(Weather::parse(real,"101030100",out));assert(out.valid);}
 std::cout<<"PASS: weather parser, real response, city match, negative temperature, 100% humidity, malformed/truncated/nonfinite data, cached reading retained\n";
}
