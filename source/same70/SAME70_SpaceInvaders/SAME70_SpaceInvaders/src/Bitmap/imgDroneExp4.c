////////////////////////////////////////////
/*

Simple Image Conversion Utility

Image Name: imgDroneExp4.bmp
MonoChrome Image 1 Bit Per Pixel
Width: 24
Height: 10
Pixel Format: Format32bppArgb
*/
///////////////////////////////////////////////////



#include <stdlib.h>
#include "bitmap.h"

static const uint8_t _acimgDroneExp4Bmp[] =
{
0x00, 0x00, 0x00, 0x08, 0x00, 0x10, 0x20, 0x22, 0x0A, 0x00,
0x00, 0x00, 0x08, 0x00, 0x20, 0x10, 0x00, 0x00, 0x00, 0x20,
0x00, 0x40, 0x10, 0x00, 0x00, 0x80, 0x20, 0x00, 0x00, 0x00};


const ImageData bmimgDroneExp4Bmp = {
24, //xSize
10, //ySize
3, //bytesPerLine
1, //bits per pixel
(uint8_t*)_acimgDroneExp4Bmp,
};
/////////////////// End of File  ///////////////////////////
