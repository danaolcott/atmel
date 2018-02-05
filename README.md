# Atmel

This repository contains a series of programs that initialize and demo various peripherals on several Atmel processors.  Programs are compiled using avr-gcc on linux and flashed using Avrdude.  The programmer used to flash the processor is an Arduino board configured as ISP (See Examples->ArduinoISP in IDE).  The hope is to create a collection of files / functions that create similar but lighter weight functionality to the Arduino IDE.

Several of the projects have an eclipse_linux folder that contains the eclipse project.  I've had a difficult time generating a hex file in eclipse that matches that using the commandline build in the Makefile.  Therefore, I've been using the hex file generated in the commandline build, and eclipse as an assistent for formatting, auto-complete, etc. 

The following is a list of peripheral projects:

ATTiny85
--------

- blink: Flashes PB3 and PB4.  Uses timer to create the timebase for delay function.
- interrupt: A simple program that toggles PB3 and PB4 and captures interrupts on PB1 and PB2 (PCINT1, PCINT2).
- moore: A simple Moore FSM with several states that flash leds on PB3 and PB4.  User buttons on PB1 and PB2 control the state.  Button presses are captured on interrupts.  Uses a timer to create the timebase for the delay function.  See photo below for prototype (blue led is a bit obnoxious and makes one a bit quezzy to look at it).
- tempSensor: A continuation of the moore state machine program that uses the TMP36GZ (or similar) temperature sensor.  The state machine contains 4 states: Calibration, flash red / blue to show relative temperature, absolute temperature (F), and ADC reading.  See main.c for more details.  No schematic is included, but there should be enough text to reproduce the board.  This makes a great center piece on any coffee table :).  

Programming the ATTiny85
------------------------
The ATTiny85 can be programmed using an Arduino as the ISP.  There's a handful of writups how to do it, but here's the gist:
- Program the Arduino with the ISP sketch located in Examples -> ArduinoISP.  A few notes on the settings, I used the default ones but the baudrate is specific to the processor.  Use 19200 for the ATTiny85.
  - Baudrate 19200 (ATTiny85 does not use the same baud rate as the ATMega328p)
  - #define SPI_CLOCK 		(1000000/6)

- Connect the ATTiny85 to the Arduino using the following pinout:
  - Arduino           ATTiny85
    10                1 (Reset - No CS Pin Needed)
    11                5 (MOSI)
    12                6 (MISO)
    13                7 (SCK)
    5v                8 (VCC)
    Gnd               4 (GND)

- Program the Attiny85 using either the Arduino IDE or commandline using AVRDude.  See Example projects for Makefile for how to do this.

    

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


Photo: ATTiny85 TempSensor Project.  Uses the TMP36GZ temperature sensor.  Display relative and absolute temperature using LEDs flashes.
![alt text](https://raw.githubusercontent.com/danaolcott/atmel/master/pictures/attiny85_tempsensor1.jpg)



