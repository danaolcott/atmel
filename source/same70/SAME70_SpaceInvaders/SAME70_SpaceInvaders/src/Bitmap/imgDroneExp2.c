////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgDroneExp2.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgDroneExp2Bmp[] =
{
0x00, 0x00, 0x00, 0x09, 0x30, 0x10, 0x23, 0x22, 0x0A, 0x07,
0x00, 0x80, 0x0E, 0x7E, 0x20, 0x12, 0x1B, 0x84, 0x07, 0x3F,
0x2A, 0x40, 0x18, 0x00, 0x04, 0x80, 0x20, 0x00, 0x00, 0x00};


const ImageData bmimgDroneExp2Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgDroneExp2Bmp,
};
/////////////////// End of File  ///////////////////////////
