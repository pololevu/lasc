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
  ssd1306 using I2C

  Basic driver for an OLED controlled by an SSD1306 over I2C
 */

#ifndef __SSD1306_I2C_H_
#define __SSD1306_I2C_H_

/* I2C pins, ports etc */
#define SSD1306_SCL_Pin        GPIO_PIN_4
#define SSD1306_SCL_Port            GPIOB
#define SSD1306_SDA_Pin        GPIO_PIN_5
#define SSD1306_SDA_Port            GPIOB

#define SSD1306_I2C_SPEED          400000
#define SSD1306_I2C_ADDRESS          0x78

/* resolution */
#define SSD1306_LCDWIDTH              128
#define SSD1306_LCDHEIGHT              64

/* Commands */
#define SSD1306_SETCONTRAST          0x81
#define SSD1306_DISPLAYALLON_RESUME  0xA4
#define SSD1306_DISPLAYALLON         0xA5
#define SSD1306_NORMALDISPLAY        0xA6
#define SSD1306_INVERTDISPLAY        0xA7
#define SSD1306_DISPLAYOFF           0xAE
#define SSD1306_DISPLAYON            0xAF
#define SSD1306_SETDISPLAYOFFSET     0xD3
#define SSD1306_SETCOMPINS           0xDA
#define SSD1306_SETVCOMDETECT        0xDB
#define SSD1306_SETDISPLAYCLOCKDIV   0xD5
#define SSD1306_SETPRECHARGE         0xD9
#define SSD1306_SETMULTIPLEX         0xA8
#define SSD1306_SETLOWCOLUMN         0x00
#define SSD1306_SETHIGHCOLUMN        0x10
#define SSD1306_SETSTARTLINE         0x40
#define SSD1306_MEMORYMODE           0x20
#define SSD1306_COLUMNADDR           0x21
#define SSD1306_PAGEADDR             0x22
#define SSD1306_COMSCANINC           0xC0
#define SSD1306_COMSCANDEC           0xC8
#define SSD1306_SEGREMAP             0xA0
#define SSD1306_CHARGEPUMP           0x8D
#define SSD1306_EXTERNALVCC          0x01
#define SSD1306_SWITCHCAPVCC         0x02

/* Colours */
#define BLACK                        0
#define WHITE                        1
#define INVERSE                      2

#ifndef NULL
#define NULL                         (void *)0
#endif /* NULL */

/* index of first char of non digit chars in font.h font_map[] */
#define CHAR_c_IDX                   10
#define CHAR_cc_IDX                  11
#define CHAR_C_IDX                   12
#define CHAR_u_IDX                   13
#define CHAR_U_IDX                   14
#define CHAR_BLANK_IDX               15

/* Public exported functions */
void ssd1306_Init(void);
void ssd1306_DisplayChar(uint8_t pos, uint8_t c);
void ssd1306_ShowMidiChannel(uint8_t midiChannel, uint8_t range);
void ssd1306_DisplayIntensity(uint8_t intensity);
void ssd1306_ClearDisplay(void);

#endif /* __SSD1306_I2C_H_ */
