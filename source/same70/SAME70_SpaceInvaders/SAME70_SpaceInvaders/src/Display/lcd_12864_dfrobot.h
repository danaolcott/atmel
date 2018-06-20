/*
/////////////////////////////////////////////////////////////
 * Created: 6/10/2018 12:44:47 AM
 *  Author: danao

 LCD Pixel Display from DFRobot - PN DFR0287

 The display is a 128x64 graphic display with pinout
 headers to fit with a standard arduino board.  The shield
 uses the the SH1106 display driver with SPI interface and
 pins for backlight, reset, cmd/data.  The shield also has 
 a 5 position switch that connects to A0.

Link to the Mfg website:
https://www.dfrobot.com/wiki/index.php/LCD12864_Shield_SKU:DFR0287#Pin_Out

Pinout: Labels shown are for Arduino style headers.  See
pindefs.h for conversion to atmel board.
SPI Com: SCK = 13, MOSI = 11, CS = 10, CD = 9, RST = 8

Clock: 13
MOSI: 11
CS: 10
CMD/DATA: 9
Reset: 8
Backlight: 7

From Display Driver Datasheet: 
SPI: idle clock high, data on trailing edge, MSB first



/////////////////////////////////////////////////////////////
 */ 


#ifndef LCD_12864_DFROBOT_H_
#define LCD_12864_DFROBOT_H_

#include "pindefs.h"			//arduino style pin labels
#include "bitmap.h"				//ImageData data type

#define LCD_HEIGHT				64
#define LCD_WIDTH				128

#define LCD_NUM_PAGE			8
#define LCD_NUM_COL				128

#define FRAME_BUFFER_SIZE		(LCD_HEIGHT * LCD_WIDTH / 8)

#define LCD_RESET_PIN			GPIO_D8
#define LCD_CD_PIN				GPIO_D9
#define LCD_CS_PIN				GPIO_D10
#define LCD_SCK_PIN				GPIO_D13
#define LCD_MOSI_PIN			GPIO_D11
#define LCD_BACKLIGHT_PIN		GPIO_D7

/////////////////////////////////////////////////////
extern uint8_t frameBuffer[FRAME_BUFFER_SIZE];
extern uint8_t workingBuffer[FRAME_BUFFER_SIZE];


void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteDataBurst(uint8_t* data, uint16_t length);

void LCD_Config(void);
void LCD_Reset(void);
void LCD_BacklightOn(void);
void LCD_BacklightOff(void);
void LCD_BacklightToggle(void);

//command functions
void LCD_On(void);
void LCD_Off(void);

void LCD_SetPage(uint8_t page);
void LCD_SetColumn(uint8_t col);

void LCD_SetContrast(uint8_t contrast);
void LCD_SetDisplayStartLine(uint8_t line);

void LCD_Clear(uint8_t data);
void LCD_ClearPage(uint8_t page, uint8_t width, uint8_t Loffset, uint8_t value);

void LCD_ClearMemory(uint8_t* buffer, uint8_t data);
void LCD_Update(uint8_t* buffer);

//graphics functions
void LCD_DrawCharKern(uint8_t kern, uint8_t letter);
void LCD_DrawStringKern(uint8_t row_initial, uint8_t kern, const char* mystring);
void LCD_DrawStringKernLength(uint8_t row_initial, uint8_t kern, uint8_t* mystring, uint8_t length);

void LCD_PutPixel(uint16_t x, uint16_t y, uint8_t color, uint8_t update);
uint8_t LCD_ReadPixel(uint16_t x, uint16_t y, uint8_t* buffer);

void LCD_DisplayShift(int dx, int dy, uint8_t wrap, uint8_t update);
void LCD_DrawLine(int x0, int y0, int x1, int y1, uint8_t color);

void LCD_DrawBitmap(const ImageData *image, uint8_t update);
void LCD_DrawIcon(uint32_t offsetX, uint32_t offsetY, const ImageData *pImage, uint8_t update);






#endif /* LCD_12864_DFROBOT_H_ */