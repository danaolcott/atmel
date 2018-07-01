////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgDroneExp1.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgDroneExp1Bmp[] =
{
0x00, 0x00, 0x00, 0x09, 0x3F, 0x10, 0x23, 0x27, 0x8A, 0x07,
0x03, 0x00, 0x0E, 0x7F, 0x00, 0x3A, 0x1B, 0x84, 0x7F, 0x3F,
0xEE, 0x0F, 0x18, 0xF0, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00};


const ImageData bmimgDroneExp1Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgDroneExp1Bmp,
};
/////////////////// End of File  ///////////////////////////
