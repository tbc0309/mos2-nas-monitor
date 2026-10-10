#include <cassert>
#include <iostream>
#include "snmp.h"
using namespace Snmp;
Bytes response(uint32_t id,uint8_t tag,Bytes payload,uint32_t error=0){Bytes binding=oid("1.3.6.1.4.1.6574.1.2.0");append(binding,tlv(tag,payload));Bytes p=integer(id);append(p,integer(error));append(p,integer(0));append(p,tlv(0x30,tlv(0x30,binding)));Bytes b=integer(1);append(b,tlv(4,{'p','u','b','l','i','c'}));append(b,tlv(0xa2,p));return tlv(0x30,b);}
int main(){
 Value v;auto b=response(23,2,{42});assert(decode(b,23,v));assert(v.num==42&&v.oid=="1.3.6.1.4.1.6574.1.2.0");
 assert(!decode(b,24,v));assert(!v.valid&&v.text.empty());
 auto trailing=b;trailing.push_back(0);assert(!decode(trailing,23,v));assert(!decode(response(23,2,{42},2),23,v));assert(!decode(response(23,0x81,{}),23,v));
 for(size_t n=0;n<b.size();n++){Bytes truncated(b.begin(),b.begin()+n);v={};assert(!decode(truncated,23,v));}
 v={};assert(decode(response(23,0x46,{0,1,0,0,0,0,0,0,0}),23,v));assert(v.num==72057594037927936ULL);
 v={};Bytes text(200,'A');assert(decode(response(23,4,text),23,v));assert(v.text.size()==200);
 v={};assert(decode(response(23,2,{0xff}),23,v));assert(v.negative); // HDD lifetime unavailable (-1) must not terminate GETNEXT.
 size_t p=0;Node node;assert(!read(Bytes{0x30,0x80},p,2,node));p=0;assert(!read(Bytes{0x30,0x82,0xff,0xff},p,4,node));
 p=0;auto encoded=oid("1.3.6.1.4.1.6574.1.2.0");assert(read(encoded,p,encoded.size(),node));assert(oidText(encoded,node)=="1.3.6.1.4.1.6574.1.2.0");
 std::cout<<"PASS: real firmware BER parser: response, request ID, errors, exceptions, all truncations, Counter64, long BER length, malformed length, OID roundtrip\n";
}
