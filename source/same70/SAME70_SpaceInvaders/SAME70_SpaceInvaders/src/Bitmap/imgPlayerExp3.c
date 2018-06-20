////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgPlayerExp3.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgPlayerExp3Bmp[] =
{
0x44, 0x10, 0x03, 0x00, 0x02, 0x00, 0x40, 0x00, 0x58, 0x01,
0x08, 0x00, 0x08, 0x10, 0x82, 0x02, 0x02, 0x40, 0x0C, 0x05,
0x00, 0x29, 0x10, 0x00, 0x03, 0x4D, 0x24, 0x00, 0x00, 0x00};


const ImageData bmimgPlayerExp3Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgPlayerExp3Bmp,
};
/////////////////// End of File  ///////////////////////////