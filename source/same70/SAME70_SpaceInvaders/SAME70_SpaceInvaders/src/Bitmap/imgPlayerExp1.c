////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgPlayerExp1.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgPlayerExp1Bmp[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x78, 0x38,
0x18, 0xC0, 0x08, 0x3C, 0x80, 0x02, 0x2F, 0xC0, 0x0E, 0x3F,
0xC0, 0x3F, 0xFF, 0xCC, 0x7F, 0xFF, 0xFE, 0x00, 0x00, 0x00};


const ImageData bmimgPlayerExp1Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgPlayerExp1Bmp,
};
/////////////////// End of File  ///////////////////////////