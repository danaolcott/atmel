######################################################
#Makefile For Atmel ATtiny85 (Arduino) Processor
#
#8 pin, 8mhz clock - see clock calibration reg 
#8k flash and 512 bytes
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
MCU=attiny85
F_CPU=1000000L
CC=avr-gcc
STDLIB=__AVR_ATtiny85___
OBJCOPY=avr-objcopy
IDIR=/usr/lib/avr/include
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -D${STDLIB} -I. -I${IDIR}
TARGET=main
SRCS=main.c
LINUX_PORT=/dev/ttyACM2

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

#for programming arduino
#	avrdude -F -V -v -y -c arduino -p ${MCU} -P ${LINUX_PORT} -b 115200 -U flash:w:${TARGET}.hex:i

#remove F option overrides mismatch
#for programming attiny85
	avrdude -v -c avrisp -p ${MCU} -P ${LINUX_PORT} -b 19200 -i 500 -U flash:w:${TARGET}.hex


clean:
	rm -f *.bin *.hex

