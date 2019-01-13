#ifndef COMMAND__H
#define COMMAND__H

typedef struct
{
    char const *cmdString;           //match string
    char const *menuString;          //menu string
    void (*cmdPtr) (int argc, char** argv);     //function to run
}CommandStruct;


int Command_ExeCommand(int argc, char** argv);
void Command_PrintHelp(void);




#endif
