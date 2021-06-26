/*
 * This file is part of the lasc MIDI swich project.
 *
 * Copyright (C) 2021 Simon Greaves (simon@panicpants.com).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
/*
  lasc.h

  Basic MIDI PC footswitch, an homage to and improvement on the Tech21 MIDI-mouse pedal.
  Scan and debounce switches, when pressed, send associated MIDI message.

  Two or three footswitches, 3 digit LED or OLED display, MIDI out.
*/

#ifndef __LASC_H__
#define __LASC_H__
 
/* Length of time a switch needs to be down to activate (in ms) */
#define DEBOUNCE_THRESHOLD_MS 50

/* Value to indicate if display should flash */
#define START_FLASH           0x01
#define STOP_FLASH            0x00
#define FLASH_PERIOD_MS       150

#ifdef USE_EXTERNAL_LED
/* Period the LED should on/off for */
#define LED_FLASH_LEN_MS      150

/* Off-board LED */
#define LED_GPIO_PORT         (GPIOA)
#define LED_GPIO_PIN          (GPIO_PIN_1)

/* or the onboard LED
   NB this conflicts with the I2C GPIO */
/* #define LED_GPIO_PORT         (GPIOB)
   #define LED_GPIO_PIN          (GPIO_PIN_5) */

/* GPIO call to switch LED on or off */
#define EXTERNAL_LED_ON    GPIO_WriteHigh
#define EXTERNAL_LED_OFF   GPIO_WriteLow
#endif /* USE_EXTERNAL_LED */

/* Port and Pin used for UART transmit */
#define UART_TX_PORT          (GPIOD)
#define UART_TX_PIN           (GPIO_PIN_5)

/* GPIO ports and pins connected to footswitches */
#define FS_PORT               (GPIOC)
#define UP                    0x00 
#define PATCH_UP_FS_PIN       (GPIO_PIN_3)
//#define PATCH_UP_FS_PIN       (GPIO_PIN_4)
#define DOWN                  0x01
#define PATCH_DOWN_FS_PIN     (GPIO_PIN_4)
//#define PATCH_DOWN_FS_PIN     (GPIO_PIN_3)
#define MODE                  0x02
#ifdef HAS_MODE_FS
#define MODE_FS_PIN           (GPIO_PIN_7)
#else
#define MODE_FS_PIN           PATCH_UP_FS_PIN | PATCH_DOWN_FS_PIN
#endif /* HAS_MODE_FS */

/* MIDI codes */
#define MIDI_PC               0xC0  /* 1100 0000 */
#define MIDI_CC               0xB0  /* 1011 0000 */

/* EEPROM byte offsets */
#define CHANNELOFFSET         0
#define RANGEOFFSET           1
#define LASTPCMSB             2
#define LASTPCLSB             3
#define DISPLAYOFFSET         4

/* MAXRANGE - originally this was 127 (0 - 127 range) but many devices
   have more patches available in banks where the bank is set by a CC message
   followed by the PC message (still 0 - 127). The actual maximum patch number
   is defined as a range where one range may be chosen at power-up time along with
   the MIDI channel to be used. This patch number rolls over nce the first or last
   patch number is reached.

   This is implemented as a lookup array where MAXRANGE is the maximum array index
   (1 less than number of ranges). The maximum patch number (MIDI PC) is also defined.
*/
#define MAXRANGE              4
#define MAXRANGE_0            127
#define MAXRANGE_1            199
#define MAXRANGE_2            299
#define MAXRANGE_3            799
#define MAXRANGE_4            998

/* Allow footswitches to autorepeat and get faster! */
#define AUTOREPEAT_OFF        0x00
#define AUTOREPEAT_ON         0x01
#define AUTOREPEAT_FAST       0x02
#define AUTOREPEAT_FAST_AFTER 1000
#define AUTOREPEAT_AFTER_MS   300
#define AUTOREPEAT_FAST_MS    60

/* The states the physical switch can be in - 'up' or 'down' plus 'sent' which
   means the switch positiion is unchanged but the message/action it
   activates is done - this is to stop repeating the message/action without
   releasing and repressing the switch. */
typedef enum 
{
    FS_UP   = (uint8_t)0x00,
    FS_DOWN = (uint8_t)0x01,
    FS_SENT = (uint8_t)0x02
} footSwitchState_TypeDef;

/* Struct containing the data for a given footswitch */
typedef struct footSwitch_struct
{
    GPIO_Pin_TypeDef pin;
    footSwitchState_TypeDef state;
    uint32_t timeDown;
    uint32_t firstDown;
}
footSwitch_TypeDef;

/* display spcific defines mapped onto generic ones */
#if defined MAX7219SPI
#define MIN_DISPLAY_INTENSITY MAX7219_INTENSITY_1
#define MAX_DISPLAY_INTENSITY MAX7219_INTENSITY_25
#elif defined SSD1306I2C
#define MIN_DISPLAY_INTENSITY 0x00
#define MAX_DISPLAY_INTENSITY 0xFF
#endif /* defined MAX7219SPI */

#endif /* __LASC_H__ */
