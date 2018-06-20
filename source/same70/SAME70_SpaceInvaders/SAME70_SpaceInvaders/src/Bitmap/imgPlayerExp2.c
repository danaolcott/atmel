////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgPlayerExp2.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgPlayerExp2Bmp[] =
{
0x44, 0x10, 0x03, 0x00, 0x02, 0x00, 0xE0, 0x00, 0x58, 0x39,
0x08, 0xC0, 0x08, 0x14, 0x82, 0x02, 0x0E, 0xC0, 0x0E, 0x05,
0x00, 0x3F, 0xDB, 0xCC, 0x7F, 0xFF, 0xFE, 0x00, 0x00, 0x00};


const ImageData bmimgPlayerExp2Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgPlayerExp2Bmp,
};
/////////////////// End of File  ///////////////////////////