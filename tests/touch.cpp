#include <cassert>
#include <iostream>
#include "touch.h"
using namespace Touch;
uint32_t now=100;
void click(Button& b){assert(b.update(true,now)==None);assert(b.update(true,now+30)==None);assert(b.update(false,now+110)==None);assert(b.update(false,now+140)==None);now+=220;}
int main(){
 for(unsigned count=1;count<=8;count++){
  Button b;for(unsigned i=0;i<count;i++)click(b);
  assert(b.update(false,now+269)==None);
  assert(b.update(false,now+270)==(count<=8&&count!=4&&count!=7?Action(count):None));
  assert(b.update(false,now+800)==None);now+=1200;
 }
 Button isolated;click(isolated);assert(isolated.update(false,now+500)==Tap);now+=1000;click(isolated);assert(isolated.update(false,now+500)==Tap);
 Button bounce;assert(bounce.update(true,now)==None);assert(bounce.update(false,now+10)==None);assert(bounce.update(false,now+600)==None);
 Button hold;assert(hold.update(true,now)==None);hold.update(true,now+30);hold.update(false,now+5100);hold.update(false,now+5130);assert(hold.update(false,now+6000)==None);
 Button wrap;now=0xffffff00;click(wrap);click(wrap);click(wrap);assert(wrap.update(false,now+370)==SwitchMode);
 ResetConfirmation c;c.begin(100);assert(c.confirm(3,1000));assert(!c.pending);
 c.begin(100);assert(!c.confirm(2,1000));assert(!c.pending);
 c.begin(100);assert(!c.confirm(3,10100));
 c.begin(0xffffff00);c.update(0x2800);assert(!c.pending);
 std::cout<<"PASS: complete click bursts without intermediate actions, 4/7 ignored, 5 management, 6 setup, 8 reset, debounce, holds ignored, wraparound, triple confirmation and timeout\n";
}
