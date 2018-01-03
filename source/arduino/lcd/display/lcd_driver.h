#ifndef LCD_DRIVER__H
#define LCD_DRIVER__H

#include <stdint.h>
#include "register.h"

//some defines
//register defines for controlling LCD
//"or" arguments with these for set commands
#define LCD_WIDTH           ((uint8_t)128)
#define LCD_HEIGHT          ((uint8_t)64)

#define LCD_PAGE_SET		0xB0	//vertical
#define LCD_COL_SET_LSB		0x00    //horiz 4 bits
#define LCD_COL_SET_MSB		0x10    //horiz 4 bits
#define LCD_DISPLAY_ON		0xAF	//send directly
#define LCD_DISPLAY_OFF		0xAE	//send directly
#define LCD_DISPLAY_NORM	0xA6	//send directly
#define LCD_DISPLAY_REV		0xA7


//lcd extra pins - command/data and reset
#define LCD_CONTROL_PORT	(PORTD_DATA_R)
#define LCD_COMMAND_BIT		(BIT6)		//low cmd, high data
#define LCD_RESET_BIT		(BIT7)		//active low



//////////////////////////////////////
//memory - 128 x 64 / 8 1bit per pixel
#define FRAME_BUFFER_SIZE           1024
extern uint8_t frameBuffer[FRAME_BUFFER_SIZE];

///////////////////////////////////////////
void LCD_Delay(unsigned long val);

///////////////////////////////////
//data and command
void LCD_writeData(uint8_t data);
void LCD_writeCommand(uint8_t command);

////////////////////////////////////
//LCD control functions
void LCD_init(void);
void LCD_reset(void);
void LCD_on(void);
void LCD_off(void);
void LCD_clear(uint8_t value);
void LCD_clearPage(uint8_t page, uint8_t value);
void LCD_setPage(uint8_t page);
void LCD_setColumn(uint8_t col);

/////////////////////////////////////
//Graphics functions
void LCD_update(uint8_t* buffer);
void LCD_PutPixel(uint16_t x, uint16_t y, uint8_t color, uint8_t update);
uint8_t LCD_ReadPixel(uint16_t x, uint16_t y, uint8_t* buffer);


void LCD_drawLine(int x0, int y0, int x1, int y1, uint8_t color);
void LCD_DrawString(uint8_t row, uint8_t spacing, const char* mystring);
void LCD_DrawStringLength(uint8_t row, uint8_t spacing, uint8_t* buffer, uint8_t length);


#endif
