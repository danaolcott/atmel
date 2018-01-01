
#ifndef __REGISTER_H
#define __REGISTER_H



//////////////////////////////////////////
//register defines
#define PORTB_DATA_R    (*((volatile unsigned char*)0x25))
#define PORTB_DIR_R     (*((volatile unsigned char*)0x24))
#define PINB_R          (*((volatile unsigned char*)0x23))

#define PORTD_DATA_R    (*((volatile unsigned char*)0x2B))
#define PORTD_DIR_R     (*((volatile unsigned char*)0x2A))
#define PIND_R          (*((volatile unsigned char*)0x29))

#define EICRA_R         (*((volatile unsigned char*)0x69))
#define EIMSK_R         (*((volatile unsigned char*)0x3D))
#define EIFR_R          (*((volatile unsigned char*)0x3C))
#define SREG_R          (*((volatile unsigned char*)0x5F))

#define PRR_R           (*((volatile unsigned char*)0x64))

//timer 0
#define TCCR0A_R         (*((volatile unsigned char*)0x44))
#define TCCR0B_R         (*((volatile unsigned char*)0x45))
#define TIMSK0_R         (*((volatile unsigned char*)0x6E))
#define TCNT0_R          (*((volatile unsigned char*)0x46))
#define TIFR0_R          (*((volatile unsigned char*)0x15))

//usart
#define UCSR0A_R        (*((volatile unsigned char*)0xC0))
#define UCSR0B_R        (*((volatile unsigned char*)0xC1))
#define UCSR0C_R        (*((volatile unsigned char*)0xC2))
#define UBRR0L_R        (*((volatile unsigned char*)0xC4))
#define UBRR0H_R        (*((volatile unsigned char*)0xC5))

#define UDR0_R          (*((volatile unsigned char*)0xC6))

#define RXC0_FLAG        (UCSR0A_R & (1u << 7))       //rx complete flag - poll !RXC0
#define TXC0_FLAG        (UCSR0A_R & (1u << 6))       //tx complete flag - poll !TXC0

#define TXB8                0

#define __SEI        (SREG_R |= (1u << 7))


#endif



