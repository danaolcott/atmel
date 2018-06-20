////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: enemy1.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 16
Height: 16
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acenemy1Bmp[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20,
0x04, 0x40, 0x0F, 0xE0, 0x1B, 0xB0, 0x3F, 0xF8, 0x2F, 0xE8,
0x28, 0x28, 0x06, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00};


const ImageData imageEnemy1 = {
16, //xSize
16, //ySize
2, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acenemy1Bmp,
};
/////////////////// End of File  ///////////////////////////