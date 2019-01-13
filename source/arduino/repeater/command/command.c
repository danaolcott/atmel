
#include <string.h>
#include <stdio.h>

#include "command.h"
#include "usart.h"
#include "nrf24l01.h"


///////////////////////////////////////////////
//static function prototype defs
//see below for function definitions
static void cmdHelp(int argc, char** argv);
static void cmdFunction1(int argc, char** argv);
static void cmdFunction2(int argc, char** argv);
static void cmdFunction3(int argc, char** argv);



////////////////////////////////////////////
//CommandStruct commandTable

static const CommandStruct commandTable[5] = 
{
    {"?",       "Print Help",   cmdHelp},
    {"string1", "menu string1", cmdFunction1},
    {"string2", "menu string1", cmdFunction2},
    {"string3", "menu string2", cmdFunction3},
    {NULL, NULL, NULL},
};



void cmdHelp(int argc, char** argv)
{
    Usart_sendString("Help Function\r\n");
    Command_PrintHelp();
}


/////////////////////////////////////
//Send test string to nrf24l01
//0xFE, .......
void cmdFunction1(int argc, char** argv)
{
    uint8_t tx[8] = {0xFE, 0xFE, 0xFE, 0xFE,0xFE, 0xFE, 0xFE,0x01}; 
    nrf24_transmitData(0, tx, 8);
    Usart_sendString("FE in beginning  Hello From Handler Function 1!!\r\n");

}

void cmdFunction2(int argc, char** argv)
{
    uint8_t tx[8] = {0x01, 0xFE, 0xFE, 0xFE,0xFE, 0xFE, 0xFE,0xFE}; 
    nrf24_transmitData(0, tx, 8);
    Usart_sendString("FE  in end  Hello From Handler Function 2!!\r\n");
}


void cmdFunction3(int argc, char** argv)
{
    Usart_sendString("Hello From Handler Function 3!!\r\n");
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



