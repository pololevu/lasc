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
  MAX7219 using SPI - see the MAX7219 datasheet for details.

  Bits and pieces for controlling the MAX7219 and some related
  7 segment LED display stuff too since that is likely the most
  common use case right now. Can split that out later if I add
  support for matrices etc.
*/ 

#ifndef __MAX7219_SPI_H
#define __MAX7219_SPI_H

#include "stm8s_spi.h"
 
/* SPI pins, ports etc */
#define MAX7219_SPI                      SPI
#define MAX7219_CLK                      CLK_PERIPHERAL_SPI
#define MAX7219_SCK_PIN                  GPIO_PIN_5
#define MAX7219_MOSI_PIN                 GPIO_PIN_6
#define MAX7219_GPIO_PORT                GPIOC
#define MAX7219_SS_PORT                  GPIOA
#define MAX7219_SS_PIN                   GPIO_PIN_3

#define MAX7219_NUMDIGITS                8

#define MAX7219_SPACE_PAD                0x0F
#define MAX7219_ZERO_PAD                 0x00

/* in BCD Code B mode, a bcd digit represents itself but the characters
   'H', 'E', 'L', 'P' and '-' are defined here. */

#define MAX7219_DIGIT_DASH               0x0A
#define MAX7219_DIGIT_E                  0x0B
#define MAX7219_DIGIT_H                  0x0C
#define MAX7219_DIGIT_L                  0x0D
#define MAX7219_DIGIT_P                  0x0E

/* Un-coded' characters use individual segment activation.
   Segment mapping on a 7 segment display is:

     A
   F   B
     G
   E   C
     D    dp

   Setting the corresponding bit in the digit's register turns it on  
*/

#define MAX7219_SEGMENT_A                0x40
#define MAX7219_SEGMENT_B                0x20
#define MAX7219_SEGMENT_C                0x10
#define MAX7219_SEGMENT_D                0x08
#define MAX7219_SEGMENT_E                0x04
#define MAX7219_SEGMENT_F                0x02
#define MAX7219_SEGMENT_G                0x01
#define MAX7219_SEGMENT_dp               0x80

#define MAX7219_UNENCODED_0              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F
#define MAX7219_UNENCODED_1              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C
#define MAX7219_UNENCODED_2              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_3              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_4              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_5              MAX7219_SEGMENT_A|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_6              MAX7219_SEGMENT_A|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_7              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C
#define MAX7219_UNENCODED_8              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_9              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_A              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_b              MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_C              MAX7219_SEGMENT_A|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F
#define MAX7219_UNENCODED_c              MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_cc             MAX7219_SEGMENT_A|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_d              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_E              MAX7219_SEGMENT_A|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_F              MAX7219_SEGMENT_A|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_g              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_h              MAX7219_SEGMENT_C|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_H              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_J              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D
#define MAX7219_UNENCODED_L              MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F
#define MAX7219_UNENCODED_n              MAX7219_SEGMENT_C|MAX7219_SEGMENT_E|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_o              MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_oo             MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_P              MAX7219_SEGMENT_A|MAX7219_SEGMENT_B|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_r              MAX7219_SEGMENT_E|MAX7219_SEGMENT_G
#define MAX7219_UNENCODED_u              MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E
#define MAX7219_UNENCODED_U              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_E|MAX7219_SEGMENT_F
#define MAX7219_UNENCODED_y              MAX7219_SEGMENT_B|MAX7219_SEGMENT_C|MAX7219_SEGMENT_D|MAX7219_SEGMENT_F|MAX7219_SEGMENT_G

/* Registers */
#define MAX7219_NO_OP_REG                0x00

#define MAX7219_DIGIT_0_REG              0x01
#define MAX7219_DIGIT_1_REG              0x02
#define MAX7219_DIGIT_2_REG              0x03
#define MAX7219_DIGIT_3_REG              0x04
#define MAX7219_DIGIT_4_REG              0x05
#define MAX7219_DIGIT_5_REG              0x06
#define MAX7219_DIGIT_6_REG              0x07
#define MAX7219_DIGIT_7_REG              0x08

/* DECODEMODE is a bitmask */
#define MAX7219_DECODEMODE_REG           0x09
#define MAX7219_DECODE_NONE              0x00
#define MAX7219_DECODE_0                 0x01
#define MAX7219_DECODE_1                 0x02
#define MAX7219_DECODE_2                 0x04
#define MAX7219_DECODE_3                 0x08
#define MAX7219_DECODE_4                 0x10
#define MAX7219_DECODE_5                 0x20
#define MAX7219_DECODE_6                 0x40
#define MAX7219_DECODE_7                 0x80
#define MAX7219_DECODE_ALL               0xFF

#define MAX7219_INTENSITY_REG            0x0A
#define MAX7219_INTENSITY_1              0x00
#define MAX7219_INTENSITY_3              0x01
#define MAX7219_INTENSITY_5              0x02
#define MAX7219_INTENSITY_7              0x03
#define MAX7219_INTENSITY_9              0x04
#define MAX7219_INTENSITY_11             0x05
#define MAX7219_INTENSITY_13             0x06
#define MAX7219_INTENSITY_15             0x07
#define MAX7219_INTENSITY_17             0x08
#define MAX7219_INTENSITY_19             0x09
#define MAX7219_INTENSITY_21             0x0A
#define MAX7219_INTENSITY_23             0x0B
#define MAX7219_INTENSITY_25             0x0C
#define MAX7219_INTENSITY_27             0x0D
#define MAX7219_INTENSITY_29             0x0E
#define MAX7219_INTENSITY_31             0x0F

#define MAX7219_SCANLIMIT_REG            0x0B
#define MAX7219_DISPLAY_0                0x00
#define MAX7219_DISPLAY_01               0x01
#define MAX7219_DISPLAY_012              0x02
#define MAX7219_DISPLAY_0123             0x03
#define MAX7219_DISPLAY_01234            0x04
#define MAX7219_DISPLAY_012345           0x05
#define MAX7219_DISPLAY_0123456          0x06
#define MAX7219_DISPLAY_01234567         0x07

#define MAX7219_SHUTDOWN_REG             0x0C
#define MAX7219_SHUTDOWN_ON              0x00 
#define MAX7219_SHUTDOWN_MODE            MAX7219_SHUTDOWN_ON
#define MAX7219_SHUTDOWN_OFF             0x01
#define MAX7219_NORMAL_OPERATION         MAX7219_SHUTDOWN_OFF

#define MAX7219_DISPLAYTEST_REG          0x0F
#define MAX7219_DISPLAYTEST_OFF          0x00
#define MAX7219_DISPLAYTEST_ON           0x01

/* public function prototypes */
void max7219_Init(void);
void max7219_DisplayChar(uint8_t pos, uint8_t c);
void max7219_ShowMidiChannel(uint8_t midiChannel, uint8_t range);
void max7219_DisplayIntensity(uint8_t intensity);
void max7219_ClearDisplay(void);

#endif /* __MAX7219_SPI_H */
