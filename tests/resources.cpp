#include <cassert>
#include <cmath>
#include "resource_metrics.h"
int main(){assert(std::abs(Resources::memory(12095612,294128,69816,8427272)-27.318965)<.01);assert(Resources::memory(100,101,0,0)==-1);assert(Resources::memory(100,20,30,80)==0);Resources::Cpu c;for(unsigned i=0;i<8;i++)c.put(i,100);assert(c.finish()==-1);unsigned delta[]={2,0,1,87,10,0,0,0};for(unsigned i=0;i<8;i++)c.put(i,100+delta[i]);assert(std::abs(c.finish()-3)<.001);c.put(0,105);assert(c.finish()==-1);for(unsigned i=0;i<8;i++)c.put(i,0xfffffff0);assert(c.finish()==-1);for(unsigned i=0;i<8;i++)c.put(i,0x10);assert(std::abs(c.finish()-75)<.001);}
