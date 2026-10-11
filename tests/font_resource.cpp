#include <cassert>
#include <iostream>
#include "cjk_font.h"
int main(int argc,char** argv) {
  bool valid = argc < 2;
  assert(FontResource::begin() == valid);
  if (!valid) {
    assert(FontResource::coverage(Cjk::lookup('A'),0,0,2) == 0);
    std::cout << "PASS: invalid/missing resource rejected\n";
    return 0;
  }
  unsigned middle = 0;
  for (int font : {1,2,3,4}) {
    int size = Cjk::sizeOf(font);
    unsigned count = font >= 3 ? sizeof(Cjk::codes20)/2 : sizeof(Cjk::codes)/2;
    for (unsigned i=0;i<count;i++) {
      int index = int(i);
      for (int y=0;y<size;y++) for (int x=0;x<size;x++) {
        int alpha = Cjk::coverage(index,x,y,font);
        assert(alpha>=0 && alpha<=15);
        middle += alpha>0 && alpha<15;
      }
    }
  }
  assert(middle>0 && FontResource::readErrors==0);
  // A second identical lookup must reuse the glyph, rather than rereading the file.
  int i = Cjk::lookup('A');Cjk::coverage(i,0,0,2);
  auto reads = FontResource::reads;Cjk::coverage(i,1,1,2);
  assert(FontResource::reads==reads);
  assert(FontResource::begin()); // Closing/reopening invalidates old cache safely.
  std::cout << "PASS: resource CRC, gray levels, decode bounds and glyph cache\n";
}
