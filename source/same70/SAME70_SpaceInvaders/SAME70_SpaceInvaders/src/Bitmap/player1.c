////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: player1.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acplayer1Bmp[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x18, 0x00, 0x00, 0x3C, 0x00, 0x03, 0xFF, 0xC0, 0x0F, 0xFF,
0xF0, 0x3F, 0xFF, 0xFC, 0x7F, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF};


const ImageData imagePlayer1 = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acplayer1Bmp,
};
/////////////////// End of File  ///////////////////////////