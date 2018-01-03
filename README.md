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

- blink: Flashes Pin 13 (PB5) on a delay using /util/delay.h.
- interrupt: Program that uses PB5 (Pin13 - led) and PD2 (Pin2 - interrupt INT0).  Press the user button to change the flash state.  There are 4 flash states.  The button is configured as input, pullup enabled, falling edge trigger.  Switch debounce is incorported using a simple dummy delay function.
- timer: Configures Timer0 to interrupt at about 1khz to serve as a timebase for the project.  The timer is used to create delays.  This is an extension on interrupt project, which uses a button, an led, and 4 flash states.
- usart: Configures USART on Pins PD0 and PD1 and a simple command handler.  Interrupts on rx line are enabled.  End of command message is signaled with a \n.  Incoming data uses two buffers, flip-flop with each command.
- spi: Configures spi peripheral on pins 10 to 13 (PB2-PB5).  Functions for read, write, write array.. etc are provided.  The example uses an EEPROM IC for testing the peripheral.
- lcd: Configures spi peripheral on pins 10 to 13 (PB2-PB5).  Functions for read, write, write array.. etc are provided.  The example uses an spi-enabled lcd from Electronic Assembly (Digikey PN# 1481-1055-ND) for testing the interface.  Simple graphics functions are included in the lcd driver files for put pixel, clear, draw characters.
- tasker: Tasker project that builds two tasks, sender and receiver.  The sender sends a message to the reciever to toggle an led.  The project initializes an interrupt for a user button on PD2 (Pin2), Timer0 to create the timebase for the tasker, and an led on PB5 (Pin 13).

Photo: Demo of Electronic Assembly LCD graphic display, 128x64 using the spi interface.
![alt text](https://raw.githubusercontent.com/danaolcott/atmel/master/pictures/arduino_lcd.jpg)
