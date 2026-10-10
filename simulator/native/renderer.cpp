#include <fstream>
#include <iostream>
#include <algorithm>
#define NATIVE_UI
#include "dashboard.h"
struct Recorder {
 std::ostream& out;unsigned color=0;int datum=0;
 Recorder():out(std::cout){}
 void noteText(String s,int x,int y,unsigned c,int d){out<<"text\t"<<x<<"\t"<<y<<"\t0\t"<<c<<"\t"<<d<<"\t"<<s<<"\n";}
 int textWidth(String s,int font){return s.length()*(font==4?16:font==2?8:6);}
 void setTextColor(unsigned c){color=c;}void setTextDatum(int d){datum=d;}
 void drawString(String s,int x,int y,int font){out<<"text\t"<<x<<"\t"<<y<<"\t"<<font<<"\t"<<color<<"\t"<<datum<<"\t"<<s<<"\n";}
 void fillScreen(unsigned c){out<<"rect\t0\t0\t240\t240\t"<<c<<"\n";}
 void fillRect(int x,int y,int w,int h,unsigned c){out<<"rect\t"<<x<<"\t"<<y<<"\t"<<w<<"\t"<<h<<"\t"<<c<<"\n";}
 void fillRoundRect(int x,int y,int w,int h,int r,unsigned c){out<<"round\t"<<x<<"\t"<<y<<"\t"<<w<<"\t"<<h<<"\t"<<r<<"\t"<<c<<"\n";}
 void fillCircle(int x,int y,int r,unsigned c){out<<"circle\t"<<x<<"\t"<<y<<"\t"<<r<<"\t"<<c<<"\n";}
 void drawLine(int x,int y,int xx,int yy,unsigned c){out<<"line\t"<<x<<"\t"<<y<<"\t"<<xx<<"\t"<<yy<<"\t"<<c<<"\n";}
 void drawFastHLine(int x,int y,int w,unsigned c){drawLine(x,y,x+w-1,y,c);}
 void fillTriangle(int x,int y,int a,int b,int xx,int yy,unsigned c){out<<"triangle\t"<<x<<"\t"<<y<<"\t"<<a<<"\t"<<b<<"\t"<<xx<<"\t"<<yy<<"\t"<<c<<"\n";}
};
int main(int argc,char** argv){Dashboard::State s;unsigned page=0;std::cin>>page>>s.cpu>>s.memory>>s.storage>>s.temp>>s.rx>>s.tx>>s.memoryTotal>>s.memoryAvail>>s.volumeTotal>>s.volumeUsed>>s.uptime>>s.age>>s.online>>s.rssi>>s.nas>>s.device>>s.model>>s.volume>>s.iface>>s.source>>s.historyCount;s.historyCount=std::min(30u,s.historyCount);for(unsigned i=0;i<s.historyCount;i++)std::cin>>s.rxHistory[i]>>s.txHistory[i];
 std::cin>>s.weatherEnabled>>s.weatherOnline>>s.weatherTemp>>s.humidity>>s.aqi>>s.weatherCode;std::string row;std::getline(std::cin,row);
 std::getline(std::cin,row);s.weatherCity=row;std::getline(std::cin,row);s.weatherCondition=row;std::getline(std::cin,row);s.weatherWind=row;std::getline(std::cin,row);s.weatherObserved=row;std::getline(std::cin,row);s.clock=row;std::getline(std::cin,row);s.date=row;
 std::cin>>s.alarmEnabled>>s.alarmState>>s.alarmRemaining>>s.alarmTime>>s.diskCount;s.diskCount=std::min(12u,s.diskCount);
 for(unsigned i=0;i<s.diskCount;i++)std::cin>>s.disks[i].name>>s.disks[i].model>>s.disks[i].temp>>s.disks[i].status>>s.disks[i].health;
 std::cin>>s.volumeCount;s.volumeCount=std::min(8u,s.volumeCount);for(unsigned i=0;i<s.volumeCount;i++){auto& v=s.volumes[i];std::cin>>v.name>>v.units>>v.size>>v.used;}
 std::string language="zh",color="#ffffff";std::cin>>language>>color>>s.diskGroup>>s.volumeGroup>>s.resetPending;Ui::chinese=language=="zh";unsigned rgb=std::strtoul(color.c_str()+1,nullptr,16);Dashboard::palette[Dashboard::SOFTRED]=((rgb>>19)&31)<<11|((rgb>>10)&63)<<5|((rgb>>3)&31);
 std::string minuteColor="#ffa500";std::cin>>minuteColor>>s.showSeconds>>s.weekday>>s.animationFrame;rgb=std::strtoul(minuteColor.c_str()+1,nullptr,16);Dashboard::palette[Dashboard::SOFTGREEN]=((rgb>>19)&31)<<11|((rgb>>10)&63)<<5|((rgb>>3)&31);
 for(unsigned i=0;i<s.diskCount;i++){auto& disk=s.disks[i];std::replace(disk.name.begin(),disk.name.end(),'_',' ');std::replace(disk.model.begin(),disk.model.end(),'_',' ');}
 std::string bg="#21455c";std::cin>>bg;rgb=std::strtoul(bg.c_str()+1,nullptr,16);Dashboard::palette[Dashboard::BG]=(page==5||page==6||page==7)?0x222b:((rgb>>19)&31)<<11|((rgb>>10)&63)<<5|((rgb>>3)&31);Recorder r;int tile=argc>1?std::atoi(argv[1]):240;if(tile<=0||240%tile)return 2;for(int top=0;top<240;top+=tile){Cjk::clipTop=top;Cjk::clipBottom=top+tile;if(argc>1)std::cout<<"clip\t"<<top<<"\t"<<top+tile<<"\n";Dashboard::render(r,s,page%10);}}
