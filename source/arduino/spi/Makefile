######################################################
#Makefile For Atmel ATMega328p (Arduino) Processor
#
#This makefile assumes single source file, includes 
#in base directory and /usr/lib/avr/include.
#
#Details:
#CC 		- avr-gcc -Cross compile using avr-gcc.
#STDLIB 	- set to include the appropriate .io file (see avr/io.h)
#LINUX_PORT	- not sure if all distros it shows up this way
#
#gcc flags:
#-std=c99 	- compile using c99
#-Wall		- enable all warnings. can also -pedantic to do strict ISO C
#-g		- produce debugging information
#-Os		- optimize for size (-O1,-O2,-O3..
#
#-Dsym		- macro defines
#-I		- include paths
#
#
MCU=atmega328p
F_CPU=16000000UL
CC=avr-gcc
STDLIB=__AVR_ATmega328P__
OBJCOPY=avr-objcopy
IDIR=/usr/lib/avr/include

#excluding -c -fmessage-length=0 results in output that
#makes sense.  incuding it matches output from eclipse build.

CFLAGS=-std=c99 -Wall -v -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -D${STDLIB} -I. -I${IDIR}

CFLAGS+=-I./spi_driver/

#CFLAGS=-std=c99 -Wall -c -fmessage-length=0 -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -D${STDLIB} -I. -I${IDIR}
#CFLAGS+=-I./usart/
#CFLAGS+=-I./command/



TARGET=main
#SRCS=main.c ./usart/usart.c ./command/command.c
SRCS=main.c ./spi_driver/spi.c


TARGET_EXT=./eclipse_linux/Debug/avr_spi

LINUX_PORT=/dev/ttyACM0

all:
	${CC} ${CFLAGS} -o ${TARGET}.bin ${SRCS}
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.bin ${TARGET}.hex


###############################################
#Flash processor using avrdude
#
#Options (just a few good ones):
#-F	Override invalid signature check
#-V	Do not verify
#-v	verbose output
#-y	Count number of erase cycles in eeprom
#-c	programmer type (arduino)
#-p	part number
#-P	Port
#-U	memory option specification For Example:
#
#	<memtype>:r|w|v:<filename>[:format]
#
#	Alternatives:
#	-U main.hex
#	-U flash:w:main.hex:i
#
#Either one of the following works.

flash:
#	avrdude -F -V -v -y -c arduino -p ${MCU} -P ${LINUX_PORT} -b 115200 -U ${TARGET}.hex
	avrdude -F -V -v -y -c arduino -p ${MCU} -P ${LINUX_PORT} -b 115200 -U flash:w:${TARGET}.hex:i

flashext:
	avrdude -F -V -v -y -c arduino -p ${MCU} -P ${LINUX_PORT} -b 115200 -U flash:w:${TARGET_EXT}.hex:i


clean:
	rm -f *.bin *.hex

