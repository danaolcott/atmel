/*
Dana Olcott
2015?, updated 1/2/18

LCD Driver file for the  EA DOGL128x-6 by Electronic Assembly
128 x 64 pixel graphic display with an SPI interface.
This version of the lcd driver file is written for the
ATMega328p processor (Arduino)

Datasheet shows that SPI interface is supposed to be idle clock
high, data latched on trailing edge.  But it appears to 
also work idle clock low and data on a leading edge!! 

The lcd driver file uses a frame buffer to store
the state of the display.  This is necessary because
the display is write-only.  All graphics operate
on the frame buffer, which is then dumped to the display.

Control lines:
SPI lines
cmd/data line - see .h for bit definitions
reset line - see .h for bit definitions



*/

#include <stdio.h>
#include <stdlib.h>

#include "lcd_driver.h"
#include "spi.h"
#include "register.h"
#include "font_table.h"
#include "offset.h"


///////////////////////////////////////
//Memory
uint8_t frameBuffer[FRAME_BUFFER_SIZE];


////////////////////////////////////////
void LCD_Delay(unsigned long val)
{
	volatile unsigned long temp = val;
	while (temp > 0)
		temp--;
}



//data - send 8 bits to lcd with A0
//pin high
void LCD_writeData(uint8_t data)
{
	LCD_CONTROL_PORT |= LCD_COMMAND_BIT;
	spi_write(data);
}

//command - send 8 bits to lcd with A0
//pin low
void LCD_writeCommand(uint8_t command)
{
	LCD_CONTROL_PORT &=~ LCD_COMMAND_BIT;
	spi_write(command);
}

//////////////////////////////////////
//LCD_init()
//Init the lcd registers based on Page
//6 of the datasheet.
//
void LCD_init()
{
	uint16_t i;
	for (i = 0 ; i < FRAME_BUFFER_SIZE ; i++)
		frameBuffer[i] = 0x00;

    //recommaned setup
    LCD_reset();
    LCD_Delay(5000);
    LCD_writeCommand(0x40);        //display start line 0
    LCD_writeCommand(0xA1);        //ADC reverse
    LCD_writeCommand(0xC0);        //normal output
    LCD_writeCommand(0xA6);        //display normal/reverse
    LCD_writeCommand(0xA2);        //bias
    LCD_writeCommand(0x2F);        //boost
    
    LCD_writeCommand(0xF8);
    LCD_writeCommand(0x00);
    
    //this results in good contrast
    //last command should be 0x16, error
    //in the datasheet
    LCD_writeCommand(0x27);            
    LCD_writeCommand(0x81);
    LCD_writeCommand(0x16);

    LCD_writeCommand(0xAC);
    LCD_writeCommand(0x00);
    LCD_writeCommand(0xAF);         //display on

}


void LCD_reset()
{
	LCD_CONTROL_PORT &=~ LCD_RESET_BIT;
	LCD_Delay(5000);
	LCD_CONTROL_PORT |= LCD_RESET_BIT;
}



void LCD_on(void)
{
    LCD_writeCommand(LCD_DISPLAY_ON);
}

void LCD_off(void)
{
    LCD_writeCommand(LCD_DISPLAY_OFF);    
}


////////////////////////////////////////////////
//update the contentns of a frame buffer
//as well as clear the display
void LCD_clear(uint8_t value)
{
	uint16_t i;
	for (i = 0 ; i < FRAME_BUFFER_SIZE ; i++)
		frameBuffer[i] = 0x00;

    LCD_clearPage(0, value);
    LCD_clearPage(1, value);
    LCD_clearPage(2, value);
    LCD_clearPage(3, value);
    LCD_clearPage(4, value);
    LCD_clearPage(5, value);
    LCD_clearPage(6, value);
    LCD_clearPage(7, value);    
}

void LCD_clearPage(uint8_t page, uint8_t value)
{
    uint8_t i = 0;
    uint16_t element = 0x00;

    LCD_setPage(page);
    LCD_setColumn(0);
    element = page * LCD_WIDTH;    //initial buffer element

    //write 127 bytes of value
    for (i = 0 ; i < 128 ; i++)
    {
        LCD_writeData(value);
        frameBuffer[element + i] = value;
    }
    
}

/////////////////////////////////
//Set page - sets the page address
//page is the lower 3 bits, value 0 to 7
//
void LCD_setPage(uint8_t page)
{
    uint8_t value = LCD_PAGE_SET | (page & 0x07);
    LCD_writeCommand(value);    
}

void LCD_setColumn(uint8_t col)
{
    uint8_t upper = 0x00;
    uint8_t lower = 0x00;

    //test the value
    if (col > 127)
        return;

    //split the LSB and MSB
    upper = col >> 4;		//shift out the lower 4
    lower = col & 0x0F;		//capture the lower 4

    upper |= LCD_COL_SET_MSB;		//or the command bits
    lower |= LCD_COL_SET_LSB;		//or the command bits

    //send the command
    LCD_writeCommand(lower);
    LCD_writeCommand(upper);
    
}


//////////////////////////////////////////////////
//update the display with contents of frame buffer
//ok, so 8 pages, each page 128 element long
void LCD_update(uint8_t* buffer)
{
    uint16_t counter = 0x00;
    uint16_t i, j = 0;
    
    for (i = 0 ; i < 8 ; i++)
    {
        LCD_setPage(i);         //index the page
        LCD_setColumn(0);       //reset the column
        for (j = 0 ; j < LCD_WIDTH ; j++)
        {
            LCD_writeData(buffer[counter]);           
            counter++;
        }        
    }   
}

//////////////////////////////////////////////////////////////
//Graphics - put pixel
//All Graphics opperations manipulate bits in the frame buffer
//Frame buffer is aligned vertically to match the display
//How about update the bits in the frame buffer and also
//write the page / column.  
//color - 0 or 1, uses the least sig bit
//update - 0 or 1, 0 = no update, just write to RAM
//                 1 = update display too
void LCD_PutPixel(uint16_t x, uint16_t y, uint8_t color, uint8_t update)
{
    uint16_t element = 0x00;    //frame buffer element
    uint8_t elementValue = 0x00;    
    uint8_t bitShift = 0x00;
    
    //test for valid input
    if ((x > 127) || (y > 63))
        return;

    //element the frame buffer to read / write
    element = ((y >> 3) * LCD_WIDTH) + x;
    
    //offset - MSB on bottom
    if (y < 8)
        bitShift = y;
    else if ((y % 8) == 0)
        bitShift = 0;
    else
        bitShift = y % 8;
    
    //read    
    elementValue = frameBuffer[element];
    
    //modify
    if (color == 1)
        elementValue |= (1 << bitShift);        //add 1
    else    
        elementValue &=~ (1 << bitShift);       //clear 1
     
    //write
    frameBuffer[element] = elementValue;

    //update
    if (update > 0)
    {
        //update the display
        LCD_setColumn(x);
        LCD_setPage((y >> 3));
        LCD_writeData(elementValue);
    }
}


/////////////////////////////////////////////////////////
//LCD_ReadPixel.  
//Returns the contents of the frame buffer at 
//specific x and y.  Display is write only, so no
//way to read directly from the LCD.
//reads from buffer, assumes 1bpp format
uint8_t LCD_ReadPixel(uint16_t x, uint16_t y, uint8_t* buffer)
{
    uint16_t element = 0x00;    //frame buffer element
    uint8_t elementValue = 0x00;    
    uint8_t bitShift = 0x00;
    
    //test for valid input
    if ((x > 127) || (y > 63))
        return 0;

    //element the frame buffer to read / write
    element = ((y >> 3) * LCD_WIDTH) + x;
    
    //offset - MSB on bottom
    if (y < 8)
        bitShift = y;
    else if ((y % 8) == 0)
        bitShift = 0;
    else
        bitShift = y % 8;
    
    //read and capture    
    elementValue = (buffer[element] & (1U << bitShift)) >> bitShift;
    
    //return the value - should be 0 or 1
    return elementValue;
}






void LCD_drawLine(int x0, int y0, int x1, int y1, uint8_t color)
{

    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = (dx>dy ? dx : -dy)/2, e2;

    for(;;)
    {
        //update the display
        LCD_PutPixel(x0,y0, color, 1);
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }    
}



///////////////////////////////////////////////
//LCD_DrawString
//Draws a string of characters at specified
//row with a character spacing.
//
void LCD_DrawString(uint8_t row_initial, uint8_t spacing, const char* mystring)
{
    uint8_t row = row_initial & 0x07;       //max value of row is 7    
    uint8_t count = 0;
    uint8_t position = 0;		//running position
    uint8_t temp = 0;

    unsigned char line;
    unsigned int value0;
    uint8_t width = 0x00;
    int i = 0;

    uint16_t element = 0x00;        //frame buffer element
    element = row * LCD_WIDTH;
    
	//set the x and y start positions
    LCD_setPage(row);
    LCD_setColumn(0);
    
    while ((mystring[count] != '\0') && (position < 127))
    {
        LCD_setColumn(position);
        temp = 8 - offset[mystring[count] - 32] + spacing;

        if ((position + temp) < 127)
        {
            line = mystring[count] - 27;		//ie, for char 32 " ", it's on line 5
            value0 = (line-1) << 3;

            //char width from offset
            width = 8 - offset[mystring[count] - 32];

            //loop through the width
            for (i = 0 ; i < width ; i++)
            {
                LCD_writeData(font_table[value0 + i]);
                frameBuffer[element] = font_table[value0 + i];
                element++;
            }
        
            //now write the remaining spacing between chars
            for (i = 0 ; i < spacing ; i++)
            {
                LCD_writeData(0x00);    
                frameBuffer[element] = 0x00;
                element++;
            }               
        }

        position += temp;
        count++;
    }
}


///////////////////////////////////////////////
//LCD_DrawStringLength
//Draws a string of characters at specified
//row with a character spacing.
//
void LCD_DrawStringLength(uint8_t row_initial, uint8_t spacing, uint8_t* mystring, uint8_t length)
{
    uint8_t row = row_initial & 0x07;       //max value of row is 7    
    uint8_t count = 0;
    uint8_t position = 0;		//running position
    uint8_t temp = 0;

    unsigned char line;
    unsigned int value0;
    uint8_t width = 0x00;
    int i = 0;

    uint16_t element = 0x00;        //frame buffer element
    element = row * LCD_WIDTH;
    
	//set the x and y start positions
    LCD_setPage(row);
    LCD_setColumn(0);

      for (count = 0 ; count < length ; count++)
      {

        LCD_setColumn(position);
        temp = 8 - offset[mystring[count] - 32] + spacing;

        if ((position + temp) < 127)
        {
            line = mystring[count] - 27;		//ie, for char 32 " ", it's on line 5
            value0 = (line-1) << 3;

            //char width from offset
            width = 8 - offset[mystring[count] - 32];

            //loop through the width
            for (i = 0 ; i < width ; i++)
            {
                LCD_writeData(font_table[value0 + i]);
                frameBuffer[element] = font_table[value0 + i];
                element++;
            }
        
            //now write the remaining spacing between chars
            for (i = 0 ; i < spacing ; i++)
            {
                LCD_writeData(0x00);    
                frameBuffer[element] = 0x00;
                element++;
            }               
        }

        position += temp;
    }
}


