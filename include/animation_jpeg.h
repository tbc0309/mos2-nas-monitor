#pragma once
namespace OriginalJpeg {
#include "original_animation/i0.h"
#include "original_animation/i1.h"
#include "original_animation/i2.h"
#include "original_animation/i3.h"
#include "original_animation/i4.h"
#include "original_animation/i5.h"
#include "original_animation/i6.h"
#include "original_animation/i7.h"
#include "original_animation/i8.h"
#include "original_animation/i9.h"
inline void draw(unsigned frame){switch(frame%10){case 0:TJpgDec.drawJpg(160,160,i0,sizeof(i0));break;case 1:TJpgDec.drawJpg(160,160,i1,sizeof(i1));break;case 2:TJpgDec.drawJpg(160,160,i2,sizeof(i2));break;case 3:TJpgDec.drawJpg(160,160,i3,sizeof(i3));break;case 4:TJpgDec.drawJpg(160,160,i4,sizeof(i4));break;case 5:TJpgDec.drawJpg(160,160,i5,sizeof(i5));break;case 6:TJpgDec.drawJpg(160,160,i6,sizeof(i6));break;case 7:TJpgDec.drawJpg(160,160,i7,sizeof(i7));break;case 8:TJpgDec.drawJpg(160,160,i8,sizeof(i8));break;case 9:TJpgDec.drawJpg(160,160,i9,sizeof(i9));break;}}
}
