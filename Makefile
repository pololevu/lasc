# Makefile for stm8s code
#

PROGBASENAME=lasc
PROGNAME=$(PROGBASENAME).ihx

# DEVICE is used to identify which family the target belongs to, affects compilation.
# The periph library needs to be compiled with the same flag.
DEVICE=STM8S103

# PART is the actual chip sub type - this is used by the flash tool for memory map etc.
PART=stm8s103f3

STDPERIPHBASE=/home/simon/dvt/stm8/stm8s-sdcc
LDFLAGS=stm8s_$(DEVICE)_StdPeriph.lib -L$(STDPERIPHBASE)/src

CC=sdcc
INCLUDE=$(STDPERIPHBASE)/inc

# Can basically optimise for speed or size. Since the device is tiny,
# makes sense to default to size optimisation but then again if the
# code is tiny, can rebuild using speed optimisation.
COPT=--opt-code-size --Werror --fomit-frame-pointer
#COPT=--opt-code-speed --fomit-frame-pointer

# Debug creates a .adb file per module which later get smooshed
# into a .cdb file for the debugger, so both the library and
# .adb files need to be available at link-time I think.
#COPT=--debug --Werror --nostdlib
#COPT=--debug --Werror --opt-code-size

# Define the actual device this is built for, see $(INCLUDE)/stm8s.h
# NB: the device determines the available peripherals so the $(REL)
#     list may need to be modified to suit.
DEFINES=-D$(DEVICE)

# Determine which display and options to compile in:
#
#  DISPLAY          - MAX7219SPI       - use max7219 + 7 segment LED display
#                   - SSD1306I2C       - use SSD1306 OLED on I2C. Mutually exclusive with above.
#
#  HAS_MODE_FS      - device has a 3rd 'MODE' footswitch.
#  RESTORELASTPC    - saves current patch number in EEPROM and restores it on reboot
#  DONTSENDBANK     - Suppress sending MIDI bank in range 0 (PC 0 - 127)
#  USE_EXTERNAL_LED - Toggle a GPIO pin in config mode, mode2 and when transmitting MIDI
#
#DISPLAY=MAX7219SPI
DISPLAY=SSD1306I2C
#
VARIANT=-D${DISPLAY}
#VARIANT=-D${DISPLAY} -DUSE_EXTERNAL_LED
#VARIANT=-D${DISPLAY} -DHAS_MODE_FS -DUSE_EXTERNAL_LED

ZEROEEPROM=zero-eeprom

CFLAGS=-mstm8 --std-sdcc11 $(DEFINES) $(VARIANT) $(COPT)

AS=sdasstm8
ASFLAGS=-plosgffwy

AR=sdar
ARFLAGS=-rc

OBJCOPY=sdobjcopy

FLASHER=stm8flash
FLASHOPTS=-c stlinkv2 -p $(PART)

ZIP=zip
ZIPFLAGS=-9qo -FS
ZIPNAME=$(PROGBASENAME)-`date +%F`.zip
ZIPLIST=Makefile README README.md COPYING TODO \
        $(PROGBASENAME).h $(PROGBASENAME).c font.h max7219-spi.h max7219-spi.c ssd1306-i2c.h ssd1306-i2c.c \
        img/*

# per display config
ifeq (${DISPLAY}, MAX7219SPI)
INC= $(PROGBASENAME).h max7219-spi.h
SRC= $(PROGBASENAME).c max7219-spi.c
REL= $(PROGBASENAME).rel max7219-spi.rel
else ifeq (${DISPLAY}, SSD1306I2C)
INC= $(PROGBASENAME).h ssd1306-i2c.h font.h
SRC= $(PROGBASENAME).c ssd1306-i2c.c
REL= $(PROGBASENAME).rel ssd1306-i2c.rel
endif


all: $(PROGNAME)

.PHONY: clean spotless flash

$(PROGNAME): $(REL)
	$(CC) $(CFLAGS) $(REL) $(LDFLAGS) -o $(PROGNAME)

%.rel : %.s
	$(AS) $(ASFLAGS) $<

%.rel : %.c %.h font.h
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $<

flash: $(PROGNAME)
	$(FLASHER) $(FLASHOPTS) -w $(PROGNAME)

# Convert to binary - good for checking size
binary: $(PROGNAME)
	$(OBJCOPY) -I ihex $(PROGNAME) -O binary $(PROGBASENAME).bin -S

# Zero the EEPROM - mainly for testing.
zeroeeprom:
	dd if=/dev/zero of=$(ZEROEEPROM) bs=640 count=1
	$(FLASHER) $(FLASHOPTS) -s eeprom -w $(ZEROEEPROM)
	rm -f $(ZEROEEPROM)

clean:
	@rm -f *.asm *.lst *.rel *.sym *~
	@rm -f *.lk *.map *.mem *.rst
	@rm -f $(ZEROEEPROM)

spotless: clean
	@rm -f *.adb *.cdb *.bin
	@rm -f $(PROGNAME)

zip: spotless
	@$(ZIP) $(ZIPFLAGS) $(ZIPNAME) $(ZIPLIST)
	@$(ZIP) -T $(ZIPNAME)

test:
	@clear
	@amidi -p hw:1,0,0 -d

love:
	@echo 'not war!'
