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
   This module contains the lasc display functions for the max7219
   7 segment LED driver IC.

   The lasc neede:
    - init routine, setup GPIO, initialise hardware/display
    - display a 3 digit number
    - set display intensity/brightness (used to 'flash' the display)
    - display MIDI channel (2 digits) and 'range' character
    
*/

#include "stm8s.h"
#include "max7219-spi.h"

static void max7219_SPISendData(uint8_t regAddr, uint8_t data);
static void initSpi(void);

extern void delayMs(uint16_t ms);

/*---------------------------------------------------------------------------*/

/* rangeChar is used to show which range is selected. This is a single character. */
static uint8_t rangeChar[] = {
    MAX7219_UNENCODED_c,
    MAX7219_UNENCODED_cc,
    MAX7219_UNENCODED_C,
    MAX7219_UNENCODED_u,
    MAX7219_UNENCODED_U 
};

void max7219_Init(void) 
{
    /* Configure SPI GPIO pins: SCK and MOSI */
    GPIO_Init(MAX7219_GPIO_PORT,(GPIO_Pin_TypeDef)(MAX7219_SCK_PIN | MAX7219_MOSI_PIN),
              GPIO_MODE_OUT_PP_LOW_FAST);

    /* SS (aka CS or LOAD) pin */ 
    GPIO_Init(MAX7219_SS_PORT,(GPIO_Pin_TypeDef)MAX7219_SS_PIN,
              GPIO_MODE_OUT_PP_LOW_FAST);;
    
    /* Enable SPI clock */
    CLK_PeripheralClockConfig(MAX7219_CLK, ENABLE);;

    /* Initialize SPI */
    SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_2, SPI_MODE_MASTER, 
             SPI_CLOCKPOLARITY_LOW, SPI_CLOCKPHASE_1EDGE, SPI_DATADIRECTION_1LINE_TX, 
             SPI_NSS_SOFT, 0x07);
    SPI_Cmd(ENABLE);;

    /* The MAX7219 needs a brief delay before it can be accessed */
    delayMs(500);
    
    /* On initial power-up, all control registers are reset, the
       display is blanked, and the MAX7219 enters
       shutdown mode. Let's fix that...
    */

    /* Set no display test */
    max7219_SPISendData(MAX7219_DISPLAYTEST_REG, MAX7219_DISPLAYTEST_OFF);
  
    /* Set scan limit register to 3 digits. Remember to check the ISET resistor */
    max7219_SPISendData(MAX7219_SCANLIMIT_REG, MAX7219_DISPLAY_012);

    /* Set intensity register */
    max7219_SPISendData(MAX7219_INTENSITY_REG, MAX7219_INTENSITY_25);
  
    /* Clear data */
    max7219_ClearDisplay();
    
    /* Set shutdown register to 'normal' */
    max7219_SPISendData(MAX7219_SHUTDOWN_REG, MAX7219_NORMAL_OPERATION);;
}

void max7219_DisplayChar(uint8_t pos, uint8_t c)
{
    max7219_SPISendData(pos, c);
}

/* Display MIDI channel and range character */
void max7219_ShowMidiChannel(uint8_t midiChannel, uint8_t range)
{
    uint8_t displayChannel = midiChannel+1;
    
    /* Set Digit0 to be undecoded */
    max7219_SPISendData(MAX7219_DECODEMODE_REG, 0xFE);

    /* Display appropriate range character */
    max7219_SPISendData(MAX7219_DIGIT_0_REG, rangeChar[range]);
    
    if (displayChannel > 9)
        {
            max7219_SPISendData(MAX7219_DIGIT_2_REG, 1);
            max7219_SPISendData(MAX7219_DIGIT_1_REG, displayChannel - 10);
        }
    else 
        {
            max7219_SPISendData(MAX7219_DIGIT_2_REG, 0x0F);
            max7219_SPISendData(MAX7219_DIGIT_1_REG, displayChannel);
        }
}

void max7219_DisplayIntensity(uint8_t intensity) 
{
    max7219_SPISendData(MAX7219_INTENSITY_REG, intensity);
}

void max7219_ClearDisplay(void)
{
    register uint8_t i;

    /* set decode-mode register */
    max7219_SPISendData(MAX7219_DECODEMODE_REG, MAX7219_DECODE_ALL);

    for (i = 1; i <= MAX7219_NUMDIGITS; i++)
        {
            max7219_SPISendData(i, 0x0F);
        };
}

/* Send data through the SPI peripheral */
static void max7219_SPISendData(uint8_t regAddr, uint8_t data)
{
    /*  Data bits are labeled D0–D15 (Table 1).
        D8–D11 contain the register address. D0–D7 contain
        the data, and D12–D15 are “don’t care” bits. The first
        received is D15, the most significant bit (MSB).
    */
    /* set CS low */
    GPIO_WriteLow(MAX7219_SS_PORT, (GPIO_Pin_TypeDef)MAX7219_SS_PIN);
    
    /* Send register address followed by data bits through the SPI peripheral */
    SPI_SendData(regAddr & 0x0F);
    while (SPI_GetFlagStatus(SPI_FLAG_TXE) == 0);
    SPI_SendData(data);
    while (SPI_GetFlagStatus(SPI_FLAG_TXE) == 0);

    /* set CS high */
    GPIO_WriteHigh(MAX7219_SS_PORT, (GPIO_Pin_TypeDef)MAX7219_SS_PIN);
}
