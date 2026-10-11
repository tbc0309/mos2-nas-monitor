#include <cassert>
#include "startup_plan.h"
int main() {
  assert(!Sampling::storageBootstrapReady(60000, 7, 0));
  assert(Sampling::storageBootstrapReady(10000, 15, 0));
  assert(Sampling::storageBootstrapReady(0, 0, 8));
  assert(Sampling::storageBootstrapReady(0, 0, 9));
  assert(Sampling::storageBootstrapReady(120000, 0, 0));
  using namespace Sampling;
  StartupPlan p;
  p.reset(100,6); assert(!p.ready(100,0) && p.ready(20100,0));
  p.promote(2000,0); for(unsigned i=0;i<4;++i) assert(p.ready(2000,i));
  p.promote(2500,6); assert(p.ready(2500,0)); // Never delay jobs again.
  for(unsigned page=0;page<10;++page) for(unsigned g=0;g<9;++g) assert(startupDelay(page,g)<=70000);
  assert(startupDelay(1,2)==0 && startupDelay(1,8)==0);
  assert(startupDelay(2,0)==0 && startupDelay(2,1)==0);
  assert(startupDelay(4,4)==0 && startupDelay(4,5)==0);
  assert(startupDelay(8,6)==0 && startupDelay(9,7)==0);
  p.reset(0xfffffff0,6); assert(!p.ready(0,0) && p.ready(19984,0));
}
