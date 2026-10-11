#include "firmware_image.h"
#include <cassert>
#include <vector>
#include <cstdio>
uint32_t checksum(const std::vector<uint8_t>& b) {
  uint32_t crc=~0u;
  for(auto x:b){crc^=x;for(int i=0;i<8;i++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));}
  return crc^~0u;
}
int main(){
  std::vector<uint8_t> bin(5000,0xff);bin[0]=0xe9;
  const uint32_t resource=0xf1db8380;
  for(int i=0;i<16;i++)bin[4090+i]=FirmwareImage::contractByte(i,resource);
  for(int chunk: {1,7,512,4096,5000}) {
    FirmwareImage::Validator v;v.begin(bin.size(),checksum(bin),resource);
    for(size_t i=0;i<bin.size();i+=chunk)assert(v.add(bin.data()+i,std::min(size_t(chunk),bin.size()-i)));
    assert(v.complete());
  }
  FirmwareImage::Validator v;v.begin(bin.size(),checksum(bin),resource);v.add(bin.data(),bin.size()-1);assert(!v.complete());
  v.begin(bin.size(),checksum(bin),resource+1);v.add(bin.data(),bin.size());assert(!v.complete());
  v.begin(bin.size(),checksum(bin)^1,resource);v.add(bin.data(),bin.size());assert(!v.complete());
  bin[0]=0;v.begin(bin.size(),checksum(bin),resource);assert(!v.add(bin.data(),bin.size()));
  v.begin(bin.size()-1,checksum(bin),resource);assert(!v.add(bin.data(),bin.size()));
  v.begin(0x100001,0,resource);assert(!v.valid);
  puts("PASS: OTA length, magic, CRC, layout/resource compatibility and chunk boundaries");
}
