# Atmel
Programs for Atmel processors that avoid the use of the Arduino IDE.

This repository contains a series of programs that initialize and demo various peripherals on several Atmel processors, including the popular ATmega328p found on the Arduino board.  Programs are compiled using avr-gcc on linux and flashed using Avrdude.  I've found this approach results in code sizes much smaller than using the Arduino IDE (although not nearly as straightforward).  The hope is to create a library of files / functions to create similar, but lighter weight functionality.

