#include <cassert>
#include <cmath>
#include "resource_metrics.h"
int main() {
  uint32_t updated = 100;
  assert(!Resources::expireSample(30100, updated, 30000));
  assert(Resources::expireSample(30101, updated, 30000) && updated == 0);
  // Expire only once: subsequent loops must allow a new network baseline.
  assert(!Resources::expireSample(30102, updated, 30000));
  updated = 0xfffffff0;
  assert(!Resources::expireSample(0x10, updated, 32));
  assert(Resources::expireSample(0x11, updated, 32));
  assert(std::abs(Resources::memory(12095612,294128,69816,8427272)-27.318965)<.01);
  assert(Resources::memory(100,101,0,0)==-1);
  assert(Resources::memory(100,20,30,80)==0);
  Resources::Cpu c;
  for(unsigned i=0;i<8;i++) c.put(i,100);
  assert(c.finish()==-1);
  unsigned delta[]={2,0,1,87,10,0,0,0};
  for(unsigned i=0;i<8;i++) c.put(i,100+delta[i]);
  assert(std::abs(c.finish()-3)<.001);
  c.put(0,105);
  assert(c.finish()==-1);
  // Recover on the next complete sample after one missing counter.
  for(unsigned i=0;i<8;i++) c.put(i,100+3*delta[i]);
  assert(std::abs(c.finish()-3)<.001);
  for(unsigned i=0;i<8;i++) c.put(i,0xfffffff0);
  assert(c.finish()==-1);
  for(unsigned i=0;i<8;i++) c.put(i,0x10);
  assert(std::abs(c.finish()-75)<.001);
}
