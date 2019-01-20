
#include <string.h>
#include <stdio.h>

#include "command.h"
#include "usart.h"
#include "nrf24l01.h"
#include "eeprom.h"

///////////////////////////////////////////////
//static function prototype defs
//see below for function definitions
static void cmdHelp(int argc, char** argv);
static void cmdEEPROMRead(int argc, char** argv);



////////////////////////////////////////////
//CommandStruct commandTable

static const CommandStruct commandTable[3] = 
{
    {"?",       "Print Help",   cmdHelp},
    {"eeprom", "write eeprom to uart", cmdEEPROMRead},

    {NULL, NULL, NULL},
};



void cmdHelp(int argc, char** argv)
{
    Usart_sendString("Help Function\r\n");
    Command_PrintHelp();
}




//////////////////////////////////////////////
//Print the contents of pages 0 - 15 out to
//usart.
void cmdEEPROMRead(int argc, char** argv)
{
    uint8_t length = 0x00;
    uint8_t buffer[128] = {0x00};
    uint8_t rxBuffer[PAGE_SIZE];
    int i, n = 0;
    Usart_sendString("Print Contents of EEPROM to USART\r\n");

    for (i = 0 ; i < PAGE_MAX + 1 ; i++)
    {
        eeprom_readPage(i, rxBuffer);

        //rxBuffer holds the eeprom contents, buffer is the 
        //readable form for rxBuffer
        length =  utility_data2HexBuffer(rxBuffer, PAGE_SIZE, buffer);

        Usart_sendArray(buffer, length);
        Usart_sendString("\r\n");
       
    }
}



/////////////////////////////////////////////
//Command_ExeCommand
//Takes parsed arguments, searches for the
//matching command string, runs the cooresponding
//function.
//returns index of the table element, -1 if
//no match
int Command_ExeCommand(int argc, char** argv)
{
    int i = 0x00;
    
    while (commandTable[i].cmdString != NULL)
    {
        if (!strcmp(argv[0], commandTable[i].cmdString))
        {
            //run the function
            commandTable[i].cmdPtr(argc, argv);
            return i;
        }

        i++;
    }

    return -1;
}


//////////////////////////////////
//Prints a listing of all command
//match string and description
void Command_PrintHelp(void)
{
    int i = 0x00;
    int n = 0x00;
    char buffer[64];

    while (commandTable[i].cmdString != NULL)
    {
        n = snprintf(buffer, 64, "%-16s  %-32s\r\n", commandTable[i].cmdString, commandTable[i].menuString);
        Usart_sendArray((unsigned char*)buffer, n);
        i++;
    }

}



