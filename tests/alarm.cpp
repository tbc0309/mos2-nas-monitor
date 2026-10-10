#include <cassert>
#include "alarm.h"
int main(){Alarm::Clock a;unsigned epoch=1800000000;
 assert(!a.update(false,420,420,20270101,epoch,5));
 assert(a.update(true,420,420,20270101,epoch,5));assert(a.state==Alarm::Ringing);
 a.tap(1000,epoch,5);assert(a.state==Alarm::Snoozed&&a.due==epoch+300);
 a.update(true,421,420,20270101,epoch+299,5);assert(a.state==Alarm::Snoozed);
 a.update(true,425,420,20270101,epoch+300,5);assert(a.state==Alarm::Ringing);
 a.tap(400000,epoch+300,5);a.tap(400400,epoch+300,5);a.tap(400800,epoch+300,5);assert(a.state==Alarm::Dismissed);
 assert(!a.update(true,420,420,20270101,epoch+301,5));
 assert(a.update(true,420,420,20270102,epoch+86400,5));
 a.update(true,421,420,20270102,epoch+86460,5);assert(a.state==Alarm::Snoozed);
 a.tap(500000,epoch+86460,5);a.tap(501000,epoch+86461,5);a.tap(502000,epoch+86462,5);assert(a.state==Alarm::Snoozed);
 Alarm::Clock restored;restored.lastDay=a.lastDay;assert(!restored.update(true,420,420,20270102,epoch+86463,5));
 Alarm::Clock weekdays;assert(!weekdays.update(true,420,420,20270103,epoch+172800,5,false));assert(weekdays.lastDay==0);
 assert(weekdays.update(true,420,420,20270104,epoch+259200,5,true));weekdays.tap(1000,epoch+259200,5);
 weekdays.update(true,425,420,20270104,epoch+259500,5,false);assert(weekdays.state==Alarm::Ringing);
}
