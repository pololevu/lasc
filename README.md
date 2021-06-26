Lasc
====

_An act of adopting one policy or way of life, or choosing one
       type of item, in place of another; a change, especially a
       radical one._ 

Abstract
========
Lasc aims to be a small and flexible MIDI patch selection tool similar
in function to the popular Tech21 MIDI Mouse. The code is written in C
and targetted to an ST-Microcontrollers stm8s MCU and either a 7
segment LED or OLED display. It is intended to be cheap and simple to
make though basic soldering and coding skills will be necessary.
Software tools required are free and the hardware required to flash
the MCU is typically very cheap and readily available.

![](img/lasc.jpg?raw=true)

The bigger picture
==================
With the obligatory pretentious bit out of the way, lasc is the Irish
word for 'switch'. There are a number of compact MIDI patch change
devices (or switchers) produced by some well known and well respected
manufacturers including Tech21nyc, Rocktron, T.C. Electronic and
Disaster Control to name a few. I owned a Tech21 MIDI Mouse which is a
simple, robust device that allows the user to send MIDI patch cahnge
messages. Effective though it is, for me it had 2 major disadvantages:

1. they are a bit big and clunky and
2. they are quite expensive.

They also only support a fixed range of PC messages and rather annoyingly
enter config mode every power up.

**Cue:** Lasc - An attempt to squish the MIDI-mouse features into a
smaller box and add a few improvements.

The original had 3 footswitches and two modes, one, immediate where
hitting the UP button just switched one patch up and DOWN, one
down. Simples. The second mode flashed the display digits and would
allow selecting a patch number using the UP/DOWN buttons with patch
numbers changing more quickly the longer the switch is held
down. Hitting the 3rd MODE switch sends the selected patch and exits
the mode. Bit trickier.

Since one of the constraints on size is the space for 3 footswitches,
the obvious mod is to allow for just the two, UP and DOWN, switches
and emulate the MODE switch by pressing both UP and DOWN at the same
time. Define 'HAS_MODE_FS' in the Makefile for the 3 button 
version.

The MIDI mouse (and many other controllers) are restricted to
selecting all 'standard' patch numbers 1 - 128 - MIDI PC 00 - 127 )
however many MIDI devices support an extended range, in my case I have
Strymon devices that have 200 memories. To access this range a MIDI CC
bank number is sent before sending the PC message. The actual range of
patches in a device may not be an exact multiple of 128 but that isn't
a problem, there is provision in the code for setting a number of
maximum patch numbers and selecting between them. By default there are
5 ranges defined: 0 - 127, 0 - 199, 0 - 299, 0 - 799 and 0 - 998.

Selecting a patch in the range 1 - 128 results in Lasc sending MIDI CC
bank 0 followed by the relevant PC message. Selecting patch 129 - 200
results in Lasc sending MIDI CC bank 1 followed by the MIDI PC message
(in this case from 0 - 72). This was intended to work with Strymon
devices but should work with other devices too. Sending a MIDI CC bank
message to a device that doesn't support it should just be ignored. If
it does cause a problem, defining DONTSENDBANK will do what it
says. Obviously this will only apply to the patch range 0 - 127.

When initially powered up, the Tech21 MIDI mouse enters its config
mode. This times out after a couple of seconds but is still a delay,
particularly if the device gets reset while in use. Instead, Lasc will
only enter config mode if a switch is held down during power up. The
config itself is stored in  built in EEPROM. On first power up, the
EEPROM is empty whih gives Lasc default MIDI channel 1 and range 0 -
127 which are reasonable defaults.

Holding down a button during power-up will enter one of the config
modes described below.

If the RESTORELASTPC define is set, the MIDI patch number is saved
to EEPROM and will be restored on power up. The downside of this is
that the EEPROM has a finite lifetime of 30K writes so could wear out.  

If USE_EXTERNAL_LED is defined, Lasc toggles a GPIO on MIDI message
send and during config mode and when in mode 2. This can be connected
to an external LED. The display also blinks during the latter two
operations so the external LED is entirely optional.

Operation
=========
Lasc stores its configuration in EEPROM. On power-up it reads these
values then enters operational mode. The default configuration is MIDI
channel 1, PC 0 - 127, display 1 - 128.

To enter a configuration mode, keep a footswitch down during power-up
until the LED display and, if fiited, the external LED
flashes. Holding down the UP button will enter the MIDI config and
holding down the DOWN button will enter the display config mode.

In MIDI config mode, the current MIDI channel is displayed followed by
an upper or lower case character, the latter to indicate the range of
available patches. The defaults are:

 - lower case 'c' at the bottom of the display: 0 - 127
 - lower case 'c' at the top of the display:    0 - 199
 - upper case 'C':                              0 - 299
 - upper case 'u':                              0 - 799
 - upper case 'U':                              0 - 998

Pressing the UP or DOWN footswitch changes the MIDI channel,
pressing the MODE switch (or pressing both UP and DOWN together on a
two switch device) selects the range.

In display config mode, the display flashes and shows a '0' or a '1'
to signify the mode. '0' means that the number displayed by lasc is
the actual MIDI PC number sent, eg 0 - 127 whereas '1' means that lasc
will display a number one greater than the MIDI PC value, eg when
sending PC 00, lasc will display '1'. This is intended as a
convenience to allow the lasc display to match the controoled device
numbering.

In either config mode, after around 3 seconds with no button press,
the channel and range selected or display mode are saved to EEPROM so
they will be the default at next power on. 

The display then shows the initial patch number (or the last PC
selected before power off if RESTORELASTPC was defined) - NB
this is not transmitted. From here on, operation should be as
described above.

Build details
=============
The target MCU for this project is the ST Micro stm8s for which there
are many very small and cheap dev boards. The device used for
development was an STM8F103S3P6. The MCU board has a regulator
on-board, good for a supply of up to 15VDC so driving this from a
conventional 9vDC effect pedal supply should be fine.

The code currently supports two different displays/drivers. The first
uses a MAX7219 and 3x7 segment LED display. The second uses a 128x64
dot matrix OLED display with SSD1306 driver. The latter provides a
small, bright, sharp, low power dispay with great visibility and needs
just 4 wires however the code to drive it is much bigger. The chosen
MCU has only 8K of flash which is a challenge. Initially the code was
compiled using the free SDCC-3.8.0 compiler and that should still be
good for the 7 segment LED version but not for the OLED
version. Fortunately the SDCC-4.* compiler generates more compact
code and allows it to be shoehorned into < 8K. 

Flashing the MCU is straightforward using a cheap STLINK2 clone bought
on ebay and the free stm8flash utility from GitHub.

The project also utilises the sdcc-stm8 port of the official ST Micro
Standard Peripheral Library - also found on GitHub. Lasc code was
developed on a Linux machine but most/all tools should run on Windows
or Mac too. 

MAX7219 -> 3 digit 7 segment LED
=================================
The display driver, a MAX7219, is connected to 3 GPIO pins:

    SCK:      PC5  ->  MAX7219 pin 13
    MOSI:     PC6  ->  MAX7219 pin  1
    CS/SS:    PA3  ->  MAX7219 pin 12

It is also connected to 3v3 (pin 19) and ground (pin 4).
LED display wiring varies but the cheapo Chinese ones seem pretty
similar if not identical. Assuming the display is oriented pins facing
down and with the decimal points on the right, viewed from above,
numbered like an IC with pin 1 at top left, common cathode display.

  MAX7219 | LED pin | LED segment
  --------|---------|------------
2 | 2 | digit 3
6 | 6 | digit 1
11 | 3 | digit 2
14 | 5 | segment a
15 | 4 | segment f
16 | 1 | segment b
17 | 11 | segment g
20 | 10 | segment c
21 | 7 | segment e
23 | 8 | segment d

There is a 27K resistor from 3v3 to pin 18 and 10uF electrolytic and
0.1uF ceramic across the power supply. The MAX7219 is supposedly a 5v
device but seems to work fine on 3v3.

SSD1306 -> 128x64 OLED
======================
While these displays are available with an SPI interface, the I2C
version seems momre popular and cheaper. The display is connected to 2
GPIO pins:

    SCL:     PB4
    SDA:     PB5

It is also connected to 3v3 (pin 19) and ground (pin 4).
The I2C address is defaulted to 0x78, change SSD1306_I2C_ADDRESS if
necessary.

MIDI
====
MIDI TX is a single GPIO via a 47R resistor to pin 5 of DIN connector.
Pin 4 of the DIN is connected to 3v3 via another 47R resistor. Pin 2
is grounded (https://www.pjrc.com/teensy/td_libs_MIDI.html).

    MIDI TX:  PD5

The code supports 2 or 3 SPST momentary foot switches, each wired
between its respective GPIO and ground.

    F/S UP:   PC3
    F/S DOWN: PC4
    F/S MODE: PC7

To minimise space used, the 2 switch version emulates the MODE switch
when both buttons are pressed simultaneously. It is fairly easy to fit
the 2 switch version in a 1590B using through hole components, a 0.56"
display and stripboard. Ths SSD1306 version easily fits into a 1590A.

Construction
============
For me, the hardest part is cutting a rectangular hole in the die-cast
aluminium 'Hammond' enclosure. I used a Dremel-like tool with small
cutting disks. There are probably way better methods. For the OLED
version, since there are only a few connections it is pretty
straightforward to just run wires from the dev board GPIO vias to the
OLED, power, footswitches and MIDI port. I glued the OLED into the box
with cyanoacrylate (superglue) which worked pretty well.

For the max7219/7 segment LED version, I used a small piece of
vero/strip board and through hole components. There is a simple
diagram included. I used 'sockets' for the 7 segment display pins but
in retrospect it would probably have been simpler to solder them
direct, your mileage may vary though I'd suggest the OLED version
looks better anyway.

I did this for fun and to learn a bit more about microcontrollers. I
don't make any guarantees it will work though mine works really
well. Officially I don't offer support but if you do have a go at
making one and get stuck, get in touch. Similarly if you have any good
ideas for improvements let me know.

Simon - simon@panicpants.com
