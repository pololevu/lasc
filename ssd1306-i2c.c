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
   This module contains the lasc display functions for an OLED with
   ssd1306 driver on the I2C bus. 

   The lasc needs:
    - init routine, setup GPIO, initialise hardware/display etc
    - display a digit at a given position
    - display MIDI channel (2 digits) and 'range' character
    - set display intensity/brightness (used to 'flash' the display)
    - clear the display
*/

#include "stm8s.h"
#include "ssd1306-i2c.h"
#include "font.h"

static void I2C_CWrite(uint8_t Address, uint8_t ControlByte, uint8_t *pData, uint16_t DataSize);
static void SSD1306_Command(uint8_t com);
static void SSD1306_Init(void);
static void SSD1306_DrawCharLine(uint8_t lc, uint8_t mc, uint8_t rc);

extern void delayMs(uint16_t ms);

/*---------------------------------------------------------------------------*/

/* rangeChar is used to show which range is selected. This is a single character. */
static uint8_t rangeChar[] = {
    CHAR_c_IDX,
    CHAR_cc_IDX,
    CHAR_C_IDX,
    CHAR_u_IDX,
    CHAR_U_IDX, 
};

/* character positions */
static uint8_t startCol[] = {  0, 48,  96 };
static uint8_t endCol[]   = { 31, 79, 127 };

/* Initialisation is a two stage process, first configure the I2C
   interface on the MCU and then after a short delay, configure
   the display using that I2C interface. */
void ssd1306_Init(void) 
{
    /* Enable I2C clock */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);

    /* Configure the SCL and SDA pins */
    GPIO_Init(SSD1306_SCL_Port, SSD1306_SCL_Pin, GPIO_MODE_OUT_OD_HIZ_FAST);
    GPIO_Init(SSD1306_SDA_Port, SSD1306_SDA_Pin, GPIO_MODE_OUT_OD_HIZ_FAST);

    /* Configure I2C */
    I2C_DeInit();

    /* Set the I2C transmit parameters */
    I2C_Init(SSD1306_I2C_SPEED,
             1,
             I2C_DUTYCYCLE_2,
             I2C_ACK_CURR,
             I2C_ADDMODE_7BIT,
             I2C_MAX_INPUT_FREQ);
    I2C_Cmd(ENABLE);

    /* Brief delay before continuing to configure the display.
       Without this cold boot will usually fail or at best be
       unreliable. The actual delay required has not been tested
       and although 50ms seems to work fine, the max7219 version
       has a 500ms pause suggested in the datasheet so doing the
       same here seems reasonable */
    delayMs(500);
    
    /* Finally, configure the device */
    SSD1306_Init();
}

/* Draw a character at a given position. See font.h
   for details on the organisation of the font data */ 
void ssd1306_DisplayChar(uint8_t pos, uint8_t c)
{
    uint8_t offset = c * 5;
    uint8_t *ptr;
    
    /* Set page address */
    SSD1306_Command(SSD1306_PAGEADDR);
    SSD1306_Command(1);
    SSD1306_Command(7);

    /* set Column address */
    SSD1306_Command(SSD1306_COLUMNADDR);
    SSD1306_Command(startCol[pos]);
    SSD1306_Command(endCol[pos]);
    
    /* draw the character */
    ptr =(uint8_t *)&font_lines[font_map[offset++] * 3];
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));

    ptr =(uint8_t *)&font_lines[font_map[offset++] * 3];
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));
    
    ptr =(uint8_t *)&font_lines[font_map[offset++] * 3];
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));

    ptr =(uint8_t *)&font_lines[font_map[offset++] * 3];
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));

    ptr = (uint8_t *)&font_lines[font_map[offset++] * 3];
    SSD1306_DrawCharLine(*ptr, *(ptr+1), *(ptr+2));
}

void ssd1306_ShowMidiChannel(uint8_t midiChannel, uint8_t range)
{
    uint8_t displayChannel = midiChannel+1;
    
    /* Display appropriate range character */
    ssd1306_DisplayChar(2, rangeChar[range]);
    
    if (displayChannel > 9)
        {
            ssd1306_DisplayChar(0, 1);
            ssd1306_DisplayChar(1, displayChannel - 10);
        }
    else 
        {
            ssd1306_DisplayChar(0, CHAR_BLANK_IDX);
            ssd1306_DisplayChar(1, displayChannel);
        }
}

void ssd1306_DisplayIntensity(uint8_t intensity)
{
    SSD1306_Command(SSD1306_SETCONTRAST);
    SSD1306_Command(intensity);
}

void ssd1306_ClearDisplay(void)
{
    static uint8_t buf = 0x00;
    static register uint16_t b;
    
    /* set Column address */
    SSD1306_Command(SSD1306_COLUMNADDR);
    SSD1306_Command(0);
    SSD1306_Command(127);
    /* Set page address */
    SSD1306_Command(SSD1306_PAGEADDR);
    SSD1306_Command(0);
    SSD1306_Command(7);
    
    for (b = 0; b < 1024; b++)
        I2C_CWrite(SSD1306_I2C_ADDRESS, 0x40, (uint8_t*)&buf, 1);
}

/*-------------------------------------------------*/

static void SSD1306_Init(void)
{
    /* Display Off */
    SSD1306_Command(SSD1306_DISPLAYOFF);
    SSD1306_Command(0x00);

    SSD1306_Command(SSD1306_SETHIGHCOLUMN);
    SSD1306_Command(0x40);

    /* Horizontal Addressing Mode */
    SSD1306_Command(SSD1306_MEMORYMODE);
    SSD1306_Command(0x00);

    /* Make it bright */
    ssd1306_DisplayIntensity(0xFF);
    SSD1306_Command(SSD1306_SEGREMAP | 0x01);
    SSD1306_Command(SSD1306_COMSCANINC | (0x08 & (1 << 3)));
  
    /* Set Normal Display */
    SSD1306_Command(SSD1306_NORMALDISPLAY);
    /* Select Multiplex Ratio */
    SSD1306_Command(SSD1306_SETMULTIPLEX);
    /* Default => 0x3F (1/64 Duty)        0x1F(1/32 Duty) */
    SSD1306_Command(0x3F);
    /* Setting Display Offset */
    SSD1306_Command(SSD1306_SETDISPLAYOFFSET);
    SSD1306_Command(0x00);  /* 00H Reset */
    /* Set Display Clock */
    SSD1306_Command(SSD1306_SETDISPLAYCLOCKDIV);
    SSD1306_Command(0x80);  /* 105HZ */
    /* Set Pre-Charge period */
    SSD1306_Command(SSD1306_SETPRECHARGE);
    SSD1306_Command(0x22);

    /* Set COM Hardware Configuration */
    SSD1306_Command(SSD1306_SETCOMPINS);
    SSD1306_Command(0x12);
    /* Set Deselect Vcomh level */
    SSD1306_Command(SSD1306_SETVCOMDETECT);
    SSD1306_Command(0x40);
    /* Set Charge Pump */
    SSD1306_Command(SSD1306_CHARGEPUMP);

    /* Endable Charge Pump */
    SSD1306_Command(0x14);

    /* Entire Display ON */
    SSD1306_Command(SSD1306_DISPLAYALLON_RESUME);
    /* Display ON */
    SSD1306_Command(SSD1306_DISPLAYON);
    
    ssd1306_ClearDisplay();
}

static void SSD1306_Command(uint8_t com)
{
  I2C_CWrite(SSD1306_I2C_ADDRESS, 0x00, &com, sizeof(com));
}

/* Write to the I2C device, in this case the SSD1306 display */
static void I2C_CWrite(uint8_t Address, uint8_t ControlByte, uint8_t *pData, uint16_t DataSize)
{
  /* Check the params */
  if ((pData == NULL) || (DataSize == 0U))
    return;

  /* Check the busy flag */
  while(I2C_GetFlagStatus(I2C_FLAG_BUSBUSY)) {};

  /* Send START condition and check if uC act as a Master */
  I2C_GenerateSTART(ENABLE);
  while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)) {};

  /* Send slave address and wait for Slave ACK */
  I2C_Send7bitAddress(Address, I2C_DIRECTION_TX);
  while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {};

  /* If TX buffer is empty, send a control byte to the Slave
   * and wait for end of transmission
   */
  while((I2C_GetFlagStatus(I2C_FLAG_TXEMPTY) == RESET)) {};
  I2C_SendData(ControlByte);
  while((I2C_GetFlagStatus(I2C_FLAG_TRANSFERFINISHED) == RESET)) {};

  /* Start sending data stream */
  while(DataSize--) {
    /* In case of last byte send NACK */
    if(!DataSize) {
      I2C_AcknowledgeConfig(I2C_ACK_NONE);
    }
    I2C_SendData(*pData++);
    while((I2C_GetFlagStatus(I2C_FLAG_TRANSFERFINISHED) == RESET)) {};
  }
  /* End of transaction, put STOP condition */
  I2C_GenerateSTOP(ENABLE);
}

static void SSD1306_DrawCharLine(uint8_t lc, uint8_t mc, uint8_t rc)
{
    I2C_CWrite(SSD1306_I2C_ADDRESS, 0x40, (uint8_t*)&lc, 1);
    I2C_CWrite(SSD1306_I2C_ADDRESS, 0x40, (uint8_t*)&lc, 1);

    for (uint8_t n = 0; n< 28; n++) 
        {
            I2C_CWrite(SSD1306_I2C_ADDRESS, 0x40, (uint8_t*)&mc, 1);
        }

    I2C_CWrite(SSD1306_I2C_ADDRESS, 0x40, (uint8_t*)&rc, 1);
    I2C_CWrite(SSD1306_I2C_ADDRESS, 0x40, (uint8_t*)&rc, 1);
}
