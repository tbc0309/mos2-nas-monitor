#include <cassert>
#define NATIVE_UI
#include "language.h"
struct Canvas {
 String label;int ink=0;bool clipped=false;
 void setTextColor(unsigned){} void setTextDatum(int){} void drawString(String,int,int,int){}
 void noteText(String s,int,int,unsigned,int){label=s;}
 void fillRect(int x,int y,int w,int h,unsigned){ink+=w*h;if(x<0||y<0||x+w>240||y+h>240)clipped=true;}
};
int main(){
 for(auto code:{0x6e29u,0x5ea6u,0x786cu,0x76d8u,0x5065u,0x5eb7u,0x00b0u,0x2103u})assert(Cjk::lookup(code)>=0);
 // Decimal points sit below digits on the shared baseline.
 for(int size:{12,16}){auto center=[&](unsigned code){int i=Cjk::lookup(code),sum=0,weight=0;for(int y=0;y<size;y++)for(int x=0;x<size;x++){int a=Cjk::coverage(i,x,y,size==12?1:2);sum+=y*a;weight+=a;}assert(weight>0);return float(sum)/weight;};assert(center('.')>center('1')+2);assert(center(':')<center('.'));}
 assert(Cjk::lookup20(0x5468)>=0);assert(Cjk::lookup20(0x516d)>=0);assert(Cjk::width("周六",3)==40);
 Canvas c;Ui::chinese=true;Ui::draw(c,"Double tap: more",230,201,1,65535,TR_DATUM);assert(c.label=="双击查看更多"&&c.ink>0&&!c.clipped);
 c=Canvas();Ui::draw(c,"内存包含缓存",10,195,1,65535);assert(!c.clipped&&c.label=="内存包含缓存");
 Ui::chinese=false;c=Canvas();Ui::draw(c,"12345678901234567890123456789012345678901234567890",10,195,2,65535);assert(!c.clipped&&c.label.find("...")!=String::npos);
 c=Canvas();Ui::draw(c,"Disk 12 42°C",230,50,2,65535,TR_DATUM);assert(!c.clipped&&c.ink>0);
 c=Canvas();Ui::draw(c,"100.0%",133,124,4,65535,TR_DATUM);assert(!c.clipped&&c.ink>0);
 c=Canvas();Ui::draw(c,"255.255.255.255",10,198,2,65535);assert(!c.clipped&&c.ink>0);assert(Cjk::width("255.255.255.255",2)<=123);
 // Tiles preserve every ink pixel while excluding work outside the active tile.
 for(int font:{1,2,3,4}){Canvas full;Cjk::draw(full,"CPU 100.0% 周六",10,55,font,65535,TL_DATUM);Canvas tiles;for(int top=0;top<240;top+=30){Cjk::clipTop=top;Cjk::clipBottom=top+30;Cjk::draw(tiles,"CPU 100.0% 周六",10,55,font,65535,TL_DATUM);}assert(full.ink==tiles.ink);Cjk::clipTop=0;Cjk::clipBottom=240;}
 for(bool zh:{false,true}){Ui::chinese=zh;for(auto label:{"CONNECTING","WI-FI SETUP","MANAGEMENT","RESET DEVICE?"}){c=Canvas();Ui::draw(c,label,120,48,3,65535,MC_DATUM);assert(!c.clipped&&c.label==Ui::tr(label));assert(Cjk::width(Ui::tr(label),3)<=224);}for(auto label:{"Joining Wi-Fi...","6 taps: Wi-Fi setup","3 taps to confirm","Erases settings/photos","Cancels after 10s","Open 192.168.4.1","Scan to open settings","Touch to return","MOS2-NAS-ffffff","Key: ffffffff"}){c=Canvas();Ui::draw(c,label,120,229,2,65535,MC_DATUM);assert(!c.clipped&&c.label==Ui::tr(label));assert(Cjk::width(Ui::tr(label),2)<=224);}}
}
