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

A0 pin (CD pin) - low = cmd, high = data
aligned in pages, vertical, page 0 at top of LCD
pixels aligned LSB top, MSB bottom of page

Page 19 - starting point for register settings




/////////////////////////////////////////////////////////////
 */ 

#include <string.h>

#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "lcd_12864_dfrobot.h"
#include "spi_driver.h"
#include "font_table.h"			//fonts
#include "offset.h"				//offsets for font table
#include "bitmap.h"				//ImageData data type


////////////////////////////////////////////////////////
//Graphics Buffers
uint8_t frameBuffer[FRAME_BUFFER_SIZE];
uint8_t workingBuffer[FRAME_BUFFER_SIZE];


static void LCD_DummyDelay(uint32_t count)
{
	volatile uint32_t temp = count;
	while (temp > 0)
		temp--;
}

///////////////////////////////////////////
//Write Command to LCD
void LCD_WriteCommand(uint8_t cmd)
{
	ioport_set_pin_level(LCD_CD_PIN, false);	//CD Pin - CMD - Low
	SPI_writeByte(cmd);		
}
///////////////////////////////////////////
//Write to LCD Data register
void LCD_WriteData(uint8_t data)
{
	ioport_set_pin_level(LCD_CD_PIN, true);	//CD Pin - Data - High
	SPI_writeByte(data);
}

//////////////////////////////////////////////
//Write Data using burst write
//All data is dumped with a single chip select
//args: pointer and length
void LCD_WriteDataBurst(uint8_t* data, uint16_t length)
{
	ioport_set_pin_level(LCD_CD_PIN, true);	//CD Pin - Data - High
	SPI_writeArray(data, length);
}


/////////////////////////////////////////////////
//Configure Pins and register settings
//Pins are for reset, cmd/data, backlight
//
void LCD_Config(void)
{
	//enable peripheral clocks for ports A, C, and D
	pmc_enable_periph_clk(ID_PIOA);								//enable the clock	
	pmc_enable_periph_clk(ID_PIOC);								//enable the clock
	pmc_enable_periph_clk(ID_PIOD);								//enable the clock
	
	//configure LCD specific pins - CS pin handled in SPI_Config
	ioport_set_pin_dir(LCD_BACKLIGHT_PIN, IOPORT_DIR_OUTPUT);	//backlight
	ioport_set_pin_dir(LCD_CD_PIN, IOPORT_DIR_OUTPUT);			//CMD/Data - 0 for command???
	ioport_set_pin_dir(LCD_RESET_PIN, IOPORT_DIR_OUTPUT);		//reset - active low

	ioport_set_pin_level(LCD_BACKLIGHT_PIN, false);				//off
	ioport_set_pin_level(LCD_CD_PIN, false);					//low
	ioport_set_pin_level(LCD_RESET_PIN, false);					//hold in reset

	//release from reset
	ioport_set_pin_level(LCD_RESET_PIN, true);					//hold in reset

	LCD_DummyDelay(100000);
	LCD_Reset();
	LCD_DummyDelay(100000);

	//configure registers - from u8g graphics lib for 
	//Newhaven display

	LCD_WriteCommand(0x40);				//set display start line
//	LCD_WriteCommand(0xA1);				//ADC set to reverse - display RAM address
	LCD_WriteCommand(0xA0);				//ADC set to normal - display RAM address

//	LCD_WriteCommand(0xC0);				//common output mode - normal
	LCD_WriteCommand(0xC8);				//common output mode - reverse


	LCD_WriteCommand(0xA6);				//display normal
	LCD_WriteCommand(0xA2);				//LCD bias 1/9
//	LCD_WriteCommand(0xA3);				//LCD bias 1/7


	LCD_WriteCommand(0x2F);				//all power control circuits on
	LCD_WriteCommand(0xF8);				//set booster ratio
	LCD_WriteCommand(0x00);				//4x
	LCD_WriteCommand(0x27);				//set V0 voltage register ratio to large
	LCD_WriteCommand(0x81);				//contrast
	LCD_WriteCommand(0x00);				//recommended contrast is 8, changed to 0
	LCD_WriteCommand(0xAC);				//indicator
	LCD_WriteCommand(0x00);				//disable
	LCD_WriteCommand(0xAF);				//display on

	LCD_DummyDelay(100000);
	LCD_WriteCommand(0xA5);				//display all points
	LCD_DummyDelay(100000);
	LCD_DummyDelay(100000);
	LCD_WriteCommand(0xA4);				//normal display

	LCD_Clear(0x00);
	LCD_On();

}

void LCD_Reset(void)
{
	ioport_set_pin_level(LCD_RESET_PIN, true);
	LCD_DummyDelay(100000);
	ioport_set_pin_level(LCD_RESET_PIN, false);
	LCD_DummyDelay(100000);
	ioport_set_pin_level(LCD_RESET_PIN, true);
}


void LCD_BacklightOn(void)
{
	ioport_set_pin_level(LCD_BACKLIGHT_PIN, true);
}

void LCD_BacklightOff(void)
{
	ioport_set_pin_level(LCD_BACKLIGHT_PIN, false);
}

void LCD_BacklightToggle(void)
{
	ioport_toggle_pin_level(LCD_BACKLIGHT_PIN);
}


///////////////////////////////////////
//LCD On - Sleep off
//
void LCD_On(void)
{
	LCD_WriteCommand(0xA4);			//all points off
	LCD_WriteCommand(0xAF);			//display on
}

//////////////////////////////////////////
//equivalent of sleep on
void LCD_Off(void)
{
	LCD_WriteCommand(0xAC);		//static indicator off
	LCD_WriteCommand(0x00);		//indictor register set
	LCD_WriteCommand(0xAE);		//display off
	LCD_WriteCommand(0xA5);		//all points on
}

//////////////////////////////////////////
//Set the current row for data writes
//8 rows max - 0  to 7
//base value = 0xB0
void LCD_SetPage(uint8_t page)
{
	if (page < 8)
	{
		uint8_t value = (0xB0 | page);
		LCD_WriteCommand(value);
	}
}

//////////////////////////////////////////////////
//Set the current column for data writes
//col 0 to 127.  two registers - upper and
//lower 4 bits
void LCD_SetColumn(uint8_t col)
{
	if (col < LCD_WIDTH)
	{
		uint8_t top = (col & 0xF0) >> 4;		// or 0x10
		uint8_t bot = (col & 0x0F);				//or 0x00
		LCD_WriteCommand(top | 0x10);

		//add 4 for reverse - ADC register
		LCD_WriteCommand(bot | 0x00);			//add 4 for NHD display??
	}
}




////////////////////////////////////////
//Set Contrast - 2 byte write
//Set contrast command enable write
//Write the contrast value - 0 to 255
//with 127 the default
void LCD_SetContrast(uint8_t contrast)
{
	LCD_WriteCommand(0x81);			//set instruction mode
	LCD_WriteCommand(contrast);		//0 to 255
}


//////////////////////////////////////////////////
//Configure the address of COM0 - initial 
//starting point of display data
//0x40 | start line, where start line = 0 to 63
void LCD_SetDisplayStartLine(uint8_t line)
{
	if (line < 64)
	{
		uint8_t value = 0x40 | line;
		LCD_WriteCommand(value);
	}
}


//////////////////////////////////////////////////
//Clear LCD with a value
//column should be auto increment
void LCD_Clear(uint8_t data)
{
	LCD_ClearMemory(frameBuffer, data);

	for (int i = 0 ; i < LCD_NUM_PAGE ; i++)
	{
		LCD_SetPage(i);			//increment the page
		LCD_SetColumn(0);		//reset the column

		for (int j = 0 ; j < LCD_NUM_COL ; j++)
			LCD_WriteData(data);		//write data, address should auto increment
	}
}


/////////////////////////////////////////////////
//LCD_ClearPage(page)
//Fill single page with value
//width - number of pixels long
//Loffset - offset from LCD start edge
//value - value to write
//Also clears the page / offset in the frameBuffer
//
void LCD_ClearPage(uint8_t page, uint8_t width, uint8_t Loffset, uint8_t value)
{
	if (page < LCD_NUM_PAGE)
	{
		if ((width + Loffset) <= LCD_NUM_COL)
		{
			LCD_SetColumn(Loffset);		//reset to offset
			LCD_SetPage(page);			//set the page

			uint16_t element = (page * LCD_WIDTH) + Loffset;

			for (int i = 0 ; i < width ; i++)
			{
				frameBuffer[element + i] = value;		//update framebuffer
				LCD_WriteData(value);					//update display data
			}
		}
	}	
}

/////////////////////////////////////////
//Clear frame buffer with data
void LCD_ClearMemory(uint8_t* buffer, uint8_t data)
{
	memset(buffer, data, FRAME_BUFFER_SIZE);
}

////////////////////////////////////////////////
//Update the LCD with the contents of gFrameBuffer1
//NOTE: LCD does not wrap from one page to the next!!
//
void LCD_Update(uint8_t* buffer)
{
	uint8_t* ptr = buffer;
	
	//write data
	for (int i = 0 ; i < LCD_NUM_PAGE ; i++)
	{
		LCD_SetColumn(0x00);
		LCD_SetPage(i);

		ptr = buffer + (i * LCD_NUM_COL);		
		LCD_WriteDataBurst(ptr, LCD_NUM_COL);
	}

}



//////////////////////////////////////////////
//LCD_DrawCharKern
//helper function for drawstring kern
//uses offset table and kerning for closer
//character spacing.  assumes that the x and
//y addresses are set in the drawstringkern function
void LCD_DrawCharKern(uint8_t kern, uint8_t letter)
{
	unsigned char line;
	unsigned int value0;
	uint8_t width = 0x00;
	int i = 0;

	//get the line based on the letter
	line = letter - 27;		//ie, for char 32 " ", it's on line 5
	value0 = (line-1) << 3;

	//now, get the char width as
	//width = 8 - char offset
	width = 8 - offset[letter - 32];

	//loop through the width
	for (i = 0 ; i < width ; i++)
	{
		LCD_WriteData(font_table[value0 + i]);
	}

	//now write the remaining spacing between chars
	for (i = 0 ; i < kern ; i++)
	{
		LCD_WriteData(0x00);
	}
}


///////////////////////////////////////////////
//LCD_WriteStringKern
//implements offset and kern for controlled
//character spacing.  pass the kern and letter into
//the helper function for each character
//start at x = 0, and keep writing until the
//x coordinate  = 127 - (8 - offset) of that char
void LCD_DrawStringKern(uint8_t row_initial, uint8_t kern, const char* mystring)
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
	LCD_SetPage(row);
	LCD_SetColumn(0);
	
	while ((mystring[count] != '\0') && (position < 127))
	{
		LCD_SetColumn(position);
		temp = 8 - offset[mystring[count] - 32] + kern;

		if ((position + temp) < 127)
		{
			//put the contents of the function here, we need
			//to update the frame buffer
			//LCD_DrawCharKern(kern, mystring[count]);

			line = mystring[count] - 27;		//ie, for char 32 " ", it's on line 5
			value0 = (line-1) << 3;

			//char width from offset
			width = 8 - offset[mystring[count] - 32];

			//loop through the width
			for (i = 0 ; i < width ; i++)
			{
				LCD_WriteData(font_table[value0 + i]);
				frameBuffer[element] = font_table[value0 + i];
				element++;
			}
			
			//now write the remaining spacing between chars
			for (i = 0 ; i < kern ; i++)
			{
				LCD_WriteData(0x00);
				frameBuffer[element] = 0x00;
				element++;
			}
		}

		position += temp;
		count++;
	}
}


void LCD_DrawStringKernLength(uint8_t row_initial, uint8_t kern, uint8_t* mystring, uint8_t length)
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
	LCD_SetPage(row);
	LCD_SetColumn(0);

	for (count = 0 ; count < length ; count++)
	{
		LCD_SetColumn(position);
		temp = 8 - offset[mystring[count] - 32] + kern;

		if ((position + temp) < 127)
		{
			//put the contents of the function here, we need
			//to update the frame buffer
			//LCD_DrawCharKern(kern, mystring[count]);

			line = mystring[count] - 27;		//ie, for char 32 " ", it's on line 5
			value0 = (line-1) << 3;

			//char width from offset
			width = 8 - offset[mystring[count] - 32];

			//loop through the width
			for (i = 0 ; i < width ; i++)
			{
				LCD_WriteData(font_table[value0 + i]);
				frameBuffer[element] = font_table[value0 + i];
				element++;
			}
			
			//now write the remaining spacing between chars
			for (i = 0 ; i < kern ; i++)
			{
				LCD_WriteData(0x00);
				frameBuffer[element] = 0x00;
				element++;
			}
		}

		position += temp;
		//count++;
	}
}



//////////////////////////////////////////////////////////////
//Graphics - put pixel
//All Graphics operations manipulate bits in the frame buffer
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
		LCD_SetColumn(x);
		LCD_SetPage((y >> 3));
		LCD_WriteData(elementValue);
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



////////////////////////////////////////////////////
//Display Shift.
//Copies the current instance into working buffer.
//Shifts the contents of the working buffer back into
//the framebuffer by dx dy.  LCD is updated with contents
//of framebuffer if "update" is set to 1.
//
//Arguments:
//dx, dy - offset to shift the display
//wrap - wrap contents of the display if pixel is shifted off screen
//update - update the contents of the LCD if 1.
//
void LCD_DisplayShift(int dx, int dy, uint8_t wrap, uint8_t update)
{
	memcpy(workingBuffer, frameBuffer, FRAME_BUFFER_SIZE);

	//i and j range from border to border.
	for (int i = 0 ; i < LCD_HEIGHT ; i++)
	{
		for (int j = 0 ; j < LCD_WIDTH ; j++)
		{
			//compute the source location for j,i on frame buffer
			int prevX = j - dx;
			int prevY = i - dy;

			if (wrap == 1)
			{
				//test the range, less than zero, wrap back,
				//greater than size, roll over
				if (prevX < 0)
				prevX = prevX + LCD_WIDTH;
				if (prevX > LCD_WIDTH - 1)
				prevX = prevX - LCD_WIDTH;
				
				if (prevY < 0)
				prevY = prevY + LCD_HEIGHT;
				if (prevY > LCD_HEIGHT - 1)
				prevY = prevY - LCD_HEIGHT;
			}

			//write prevX and prevY into j,i
			uint8_t pixelValue = LCD_ReadPixel(prevX, prevY, workingBuffer);
			LCD_PutPixel(j, i, pixelValue, 0);
		}
	}

	//finally, update the display
	if (update == 1)
	{
		LCD_Update(frameBuffer);
	}

}


//////////////////////////////////////////////////
void LCD_DrawLine(int x0, int y0, int x1, int y1, uint8_t color)
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



////////////////////////////////////////////////////
//LCD_DrawBitmap(const ImageData *pImage)
//
//Draw image at starting point 0,0 into framebuffer.
//LCD is updated if the "update" arg is 1.  Image is
//assumed to be 1d array, byte aligned left to right, top
//to bottom.  ie, not the same as LCD is aligned.
//
//This function writes the bits into the framebuffer
//aligned as pages, LSB on top. 
//
void LCD_DrawBitmap(const ImageData *image, uint8_t update)
{
	int sizeX = image->xSize;
	int sizeY = image->ySize;
	int e0, e1, e2, e3, e4, e5, e6, e7;
	uint8_t in0, in1, in2, in3, in4, in5, in6, in7;
	uint8_t data = 0x00;
	
	uint16_t bitOffset = 0x00;
	int i, j;
	uint16_t element = 0x00;
	
	if ((sizeX > LCD_WIDTH) || (sizeY > LCD_HEIGHT ))
	return;
	
	for (i = 0 ; i < 8 ; i++)
	{
		//set the page and reset the column
		LCD_SetPage(i);
		LCD_SetColumn(0);

		for (j = 0 ; j < 128 ; j++)
		{
			//get the element offset in the original
			//based on the xoffset in the array
			//int e0, e1, e2, e3, e4, e5, e6, e7;

			//get the bitoffset in the original
			//this will be used to capture the bit
			//inthe original array
			//uint16_t bitOffset;

			if (j < 8)
			bitOffset = 8 - (j + 1);
			else if ((j+1) % 8 == 0)
			bitOffset = 0;
			else
			bitOffset = 8 - ((j + 1) % 8);     //a value of 8 is in the


			//this is looking better, at least it
			//accesses the first 2 pages ok
			e0 = (8 * i * 16) + (16 * 0) + (j >> 3);	//lsb
			e1 = (8 * i * 16) + (16 * 1) + (j >> 3);
			e2 = (8 * i * 16) + (16 * 2) + (j >> 3);
			e3 = (8 * i * 16) + (16 * 3) + (j >> 3);
			e4 = (8 * i * 16) + (16 * 4) + (j >> 3);
			e5 = (8 * i * 16) + (16 * 5) + (j >> 3);
			e6 = (8 * i * 16) + (16 * 6) + (j >> 3);
			e7 = (8 * i * 16) + (16 * 7) + (j >> 3);

			in0 = image->pImageData[e0];	//lsb
			in1 = image->pImageData[e1];
			in2 = image->pImageData[e2];
			in3 = image->pImageData[e3];
			in4 = image->pImageData[e4];
			in5 = image->pImageData[e5];
			in6 = image->pImageData[e6];
			in7 = image->pImageData[e7];	//msb

			in0 = ((in0 & (1 << bitOffset)) >> bitOffset) << 0;
			in1 = ((in1 & (1 << bitOffset)) >> bitOffset) << 1;
			in2 = ((in2 & (1 << bitOffset)) >> bitOffset) << 2;
			in3 = ((in3 & (1 << bitOffset)) >> bitOffset) << 3;
			in4 = ((in4 & (1 << bitOffset)) >> bitOffset) << 4;
			in5 = ((in5 & (1 << bitOffset)) >> bitOffset) << 5;
			in6 = ((in6 & (1 << bitOffset)) >> bitOffset) << 6;
			in7 = ((in7 & (1 << bitOffset)) >> bitOffset) << 7;

			//build the data byte for writing
			data = in0 | in1 | in2 | in3 | in4 | in5 | in6 | in7;

			//update framebuffer
//			LCD_WriteData(data);
			frameBuffer[element++] = data;
		}
	}

	if (update == 1)
		LCD_Update(frameBuffer);
}


///////////////////////////////////////////////////////
//Draw  icon into frameBuffer.
//pass 1 for update to update the display
//offsetX and offsetY are x and y offsets from origin
//
void LCD_DrawIcon(uint32_t offsetX, uint32_t offsetY, const ImageData *pImage, uint8_t update)
{
	uint32_t sizeX = pImage->xSize;
	uint32_t sizeY = pImage->ySize;
	uint8_t bitValue = 0x00;
	uint8_t p;
	uint32_t counter = 0x00;
	uint8_t data = 0x00;
	uint32_t x = offsetX;
	uint32_t y = offsetY;
	
	for (uint32_t i = 0 ; i < sizeY ; i++)
	{
		x = offsetX;        //reset the x offset

		//1bpp - 8 pixels per element
		for (uint32_t j = 0 ; j < sizeX / 8 ; j++)
		{
			data = pImage->pImageData[counter];
			
			//work counter 8 times
			p = 8;                  //reset p
			while (p > 0)
			{
				bitValue = (data >> (p-1)) & 0x01;
				if (!bitValue)
					LCD_PutPixel(x, y, 0, update);
				else
					LCD_PutPixel(x, y, 1, update);

				x++;            //increment the x
				p--;
			}
			counter++;
		}
		y++;        //increment the row
	}
}

