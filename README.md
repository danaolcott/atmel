# Atmel

This repository contains a series of programs that initialize and demo various peripherals on several Atmel processors, including the popular ATmega328p found on the Arduino board.  Programs are compiled using avr-gcc on linux and flashed using Avrdude.  I've found this approach results in code sizes much smaller than using the Arduino IDE (although not nearly as straightforward).  The hope is to create a library of files / functions to create similar, but lighter weight functionality.

Several of the projects have an eclipse_linux folder that contains the eclipse project.  I've had a difficult time generating a hex file in eclipse that matches that using the commandline build in the Makefile.  Therefore, I've been using the hex file generated in the commandline build, and eclipse as an assistent for formatting, auto-complete, etc. 

The following is a list of peripheral projects:

ATTiny85
--------

- blink: Example that toggles a pin
- interrupt: Configures an interrupt
- moore: Configures a Moore state machine with 4 states

ATMega328p
----------

- blink: Toggles led
- interrupt - configures an interrupt
- timer - configures a timer and a counter
- usart - configures usart with simple command handler functionality
- spi - configures spi peripheral on pins 10-13.  uses EEPROM IC to test read/write functions
- lcd - configures spi peripheral on pins 10-13.  uses 128x64 display to test the spi interface.
- tasker - configures a simple tasker to run two tasks.  the controller uses a timer on an interrupt.


Photo: Demo of Electronic Assembly LCD graphic display, 128x64 using the spi interface.
![alt text](https://raw.githubusercontent.com/danaolcott/atmel/master/pictures/arduino_lcd.jpg)
