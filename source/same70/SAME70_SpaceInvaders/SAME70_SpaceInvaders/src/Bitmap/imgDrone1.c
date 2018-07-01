////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgDrone1.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgDrone1Bmp[] =
{
0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x03, 0xE7, 0xC0, 0x07,
0xC3, 0xE0, 0x0F, 0xFF, 0xF0, 0x3B, 0xDB, 0xDC, 0x7F, 0xFF,
0xFE, 0x0F, 0x18, 0xF0, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00};


const ImageData bmimgDrone1Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgDrone1Bmp,
};
/////////////////// End of File  ///////////////////////////
