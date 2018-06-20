////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: missile1.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 8
Height: 8
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acmissile1Bmp[] =
{
0x18, 0x18, 0x18, 0x0C, 0x0C, 0x0F, 0x18, 0x18};


const ImageData imageMissile1 = {
8, //xSize
8, //ySize
1, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acmissile1Bmp,
};
/////////////////// End of File  ///////////////////////////