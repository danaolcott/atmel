/*
Bitmap - Listing of all images in the project

*/

#ifndef __BITMAP_C
#define __BITMAP_C

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>


//image data
struct ImageData{
    uint8_t xSize;              // pixels in x-direction
    uint8_t ySize;              // pixels in y-direction
    uint8_t bytesPerLine;
    uint8_t bitsPerPixel;
    const uint8_t * const pImageData;
};

typedef struct ImageData ImageData;


//bitmaps
extern const ImageData imagePlayer1;
extern const ImageData imageEnemy1;
extern const ImageData imageMissile1;

extern const ImageData bmimgPlayerExp1Bmp;
extern const ImageData bmimgPlayerExp2Bmp;
extern const ImageData bmimgPlayerExp3Bmp;
extern const ImageData bmimgPlayerExp4Bmp;


#endif
