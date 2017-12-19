#ifndef __USART_H
#define __USART_H



//////////////////////////////
//usart buffer items
#define RX_BUFFER_SIZE              64
#define ARG_BUFFER_SIZE             16


void Usart_init(unsigned long baud);
void Usart_isr(unsigned char c);
void Usart_sendByte(unsigned char data);
void Usart_sendString(char *data);
void Usart_sendArray(unsigned char *data, unsigned int length);
void Usart_processCommand(unsigned char *data, unsigned int length);
void Usart_parseArgs(char *in, int *pargc, char** argv);

#endif

