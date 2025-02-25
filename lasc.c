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

/* Footswitch that sends pre-defined MIDI messages when pressed

   Use an stm8s microcontroller to scan a number of footswitches
   (any switches really), debounce switch presses and send the
   associated MIDI message via it's built in UART.

   This device was inspired by the Tech21 MIDI-mouse patch changer. The
   original has 3 footswitches and operates in two modes; in the first,
   pressing the UP button sends the MIDI PC command to move to the
   next patch, DOWN similarly sends the PC for the previous patch
   number.

   Pressing the third footswitch engages the second mode which flashes
   the display and the UP and DOWN switches change the patch number
   displayed but dont send it until the third fooswitch is pressed again
   which also stops the display flashing. This allows jumping from one
   patch to another without switching to each intermediate one. On power-up,
   the current MIDI channel is displayed and can be changed
   by pressing the UP or DOWN footswitches. After a period without
   footswitch presses, the device saves the MIDI channel and continues.

   This code is a sort of 'homage' to the original MIDI-mouse but extends
   it with a number of improvements to functionality including selectable
   Patch ranges using MIDI CC bank switching, choice of 2 or 3 switches,
   display actual MIDI PC number or the more human friendly PC+1 value,
   choice of LED or OLED display and faster startup by skipping the config
   mode unless a switch is held at power-up. Smaller enclosures can be used
   (Hammond 1590A for example).
*/

/* OLED version is close to the 8K flash size in the default stm8s device. SDCC version
   4 does a better job of generating compact code than version 3 so prefer it. No doubt
   there are other updates too. Version 3 might be ok for the LED version at a push. */
#if __SDCC_VERSION_MAJOR < 4
#error "Use SDCC version 4 or later else generated code may be too big for device (>8K)!"
#endif

#include "stm8s.h"
#include "lasc.h"

#if defined MAX7219SPI
#include "max7219-spi.h"
#elif defined SSD1306I2C
#include "ssd1306-i2c.h"
#else
#error "Error! either MAX7219SPI or SSD1306I2C must be defined!"
#endif /* defined MAX7219SPI */

static void initClk(void);
static void initTim2(void);
static void initGpio(void);
static void initUart(void);
static void sendMidiPC(uint16_t patch);
static void unlockEeprom(void);
static void lockEeprom(void);
static uint8_t writeEepromByte(uint32_t addr, uint8_t val);
static uint8_t readEepromByte(uint32_t addr);
static void manageConfig(void);
static void configMIDI(void);
static void configDisplay(void);
static void mode2(void);
static uint8_t scanFS(uint8_t autoRepeat, uint16_t timeoutMs);

/* Footswitch config. */
#define MAXFS 3
static footSwitch_TypeDef fsArr[MAXFS] =
    {
        {PATCH_UP_FS_PIN, FS_UP, 0, 0},   /* PC3 */
        {PATCH_DOWN_FS_PIN, FS_UP, 0, 0}, /* PC4 */
        {MODE_FS_PIN, FS_UP, 0, 0},       /* PC7 */
};

/*---------------------------------------------------------------------------*/

/* MIDI channel */
static uint8_t midiChannel = 0;
static uint16_t midiPatchNo = 0;
#if defined DONTSENDBANK
static uint8_t sendMIDIBank = 0;
#else
static uint8_t sendMIDIBank = 1;
#endif /* DONTSENDBANK */

/* maxPatch is the highest patch that may be selected. Pressing up when this is
   displayed will return to the first patch (PC 0). The values
   are arbitrary and more options can be added. */
static uint16_t maxPatch[] = {MAXRANGE_0, MAXRANGE_1, MAXRANGE_2, MAXRANGE_3, MAXRANGE_4};
static uint8_t range = 0;

/* display the actual PC patch value sent instead of adding 1 (ie 0 - 127 instead of 1 - 128)
   only affects what is displayed */
static uint8_t showZeroBased = 0;

/* Time related globals */
static __IO uint16_t msTicks = 0;
static __IO uint16_t ledTicks = 0xFFFF;
static __IO uint32_t now = 0;
static __IO uint16_t flashTicks = 0;
static __IO uint8_t doFlash = 0;
static __IO uint8_t displayIntensity = MAX_DISPLAY_INTENSITY;

/* TIM2 update interrupt handler.
   Interrupt fires when TIM2 hits its 'period' value and is updated. This is
   currently every millisecond. The handler does a number of simple tasks:

     1 - Increments 'now'. This is a uint32_t and will just roll over.
     2 - Decrements msTicks which is used by DelayMs().
     3 - Sets values used when flashing the display
*/
INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
    disableInterrupts();

    TIM2_ClearITPendingBit(TIM2_IT_UPDATE);

    /* tick... rolls over */
    now++;

    if (msTicks != 0)
    {
        msTicks--;
    }

    if (doFlash)
    /* Flash the display in config mode or mode 2.
       If the external LED is used, it also flashes to
       indicate the MIDI message transmission. The control
       of those flashes is in this interrupt handler too
       but is mutually exclusive with the display flash to
       avoid inconsistent behaviour. */
    {
        switch (flashTicks)
        {
        case 0:
            if (displayIntensity == MIN_DISPLAY_INTENSITY)
            {
                displayIntensity = MAX_DISPLAY_INTENSITY;
#ifdef USE_EXTERNAL_LED
                EXTERNAL_LED_OFF(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN);
#endif /* USE_EXTERNAL_LED */
            }
            else
            {
                displayIntensity = MIN_DISPLAY_INTENSITY;
#ifdef USE_EXTERNAL_LED
                EXTERNAL_LED_ON(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN);
#endif /* USE_EXTERNAL_LED */
            }
            flashTicks = FLASH_PERIOD_MS;
            break;

        default:
            flashTicks--;
        }
    }
#ifdef USE_EXTERNAL_LED
    else
    {
        /* flash the external LED on sending a MIDI message */
        switch (ledTicks)
        {
        case 0:
            /* time's up, turn the LED off */
            EXTERNAL_LED_OFF(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN);
            break;

        default:
            ledTicks--;
        }
    }
#endif /* USE_EXTERNAL_LED */

    enableInterrupts();
}

static void flashDisplay(uint8_t action)
{
    switch (action)
    {
    case START_FLASH:
        flashTicks = FLASH_PERIOD_MS;
        doFlash = 1;
        break;

    case STOP_FLASH:
        doFlash = 0;
        // displayIntensity = MAX_DISPLAY_INTENSITY;
#if defined MAX7219SPI
        max7219_DisplayIntensity(MAX_DISPLAY_INTENSITY);
#elif defined SSD1306I2C
        ssd1306_DisplayIntensity(MAX_DISPLAY_INTENSITY);
#endif /* defined MAX7219SPI */

    default:;
    }
}

/* Blocking millisecond delay, msTicks is decremented by TIM2 interrupt */
void delayMs(uint16_t ms)
{
    /* Reload us value */
    msTicks = ms;
    /* Wait until msTick reach zero */
    while (msTicks)
        ;
}

/* Use High Speed Internal clock at 16MHz */
static void initClk(void)
{
    CLK_DeInit();

    /* Clock source */
    CLK_HSECmd(DISABLE);
    CLK_LSICmd(DISABLE);
    CLK_HSICmd(ENABLE);
    while (CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE)
    {
    };

    /* Set the core and peripherial clocks */
    CLK_ClockSwitchCmd(ENABLE);
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
    CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);

    CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI,
                          DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);

    /* Enable the preipherial clocks used */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);

    /* Disable all other clocks for now */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_AWU, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
}

/* Start TIM2 timer, 1MHz/1us tick, 1ms interrupt */
static void initTim2(void)
{
    /* TIM2 Peripheral Configuration
       Clock runs at 16MHz so prescale of 16 gives 1uS TICK
       and 1000 period gives 1mS timer interrupt */
    TIM2_DeInit();
    TIM2_TimeBaseInit(TIM2_PRESCALER_16, 1000);

    /* TIM2 counter enable */
    TIM2_Cmd(ENABLE);
    TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
}

/* Initialise GPIOs, switches are input, displays/LEDs are outputs. */
static void initGpio(void)
{
    uint8_t i;

#ifdef USE_EXTERNAL_LED
    /* Initialize external LED pin in Output Mode and turn it on. There is a short delay
       before the display lights up so this might help assure power is on. */
    GPIO_Init(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    EXTERNAL_LED_ON(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN);
#endif /* USE_EXTERNAL_LED */

    /* Set UART_TX_PIN as Output open-drain high-impedance level (UART1_Tx) */
    GPIO_Init(UART_TX_PORT, (GPIO_Pin_TypeDef)UART_TX_PIN, GPIO_MODE_OUT_OD_HIZ_FAST);

    /* Initialise all switch GPIOs as inputs with pull-ups enabled */
    for (i = 0; i < MAXFS; i++)
    {
        GPIO_Init(FS_PORT, fsArr[i].pin, GPIO_MODE_IN_PU_NO_IT);
    }
}

/* Setup the hardware UART as a MIDI out port */
static void initUart(void)
{
    UART1_DeInit();
    /* UART1 configured as follows:
        - BaudRate = 31250 baud
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Receive disabled
        - UART1 Clock disabled
  */
    UART1_Init((uint32_t)31250, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
               UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TX_ENABLE);

    UART1_Cmd(ENABLE);
}

/* Display the patch number */
static void displayPatch(uint16_t patchNo)
{
    register int8_t i;

    if (!showZeroBased)
        patchNo++;

#if defined MAX7219SPI
    max7219_ClearDisplay();

    for (i = 1; i <= MAX7219_NUMDIGITS; i++)
    {
        if (patchNo > 0)
        {
            max7219_DisplayChar(i, patchNo % 10);
            patchNo /= 10;
        }
        else if (i == 1) // special case for showZeroBased mode
        {
            max7219_DisplayChar(i, 0);
        }
        else
        {
            max7219_DisplayChar(i, MAX7219_SPACE_PAD);
        }
    }
#elif defined SSD1306I2C
    for (i = 2; i >= 0; i--)
    {
        if (patchNo > 0)
        {
            ssd1306_DisplayChar(i, patchNo % 10);
            patchNo /= 10;
        }
        else if (i == 2) // special case for showZeroBased mode
        {
            ssd1306_DisplayChar(i, 0);
        }
        else
        {
            ssd1306_DisplayChar(i, CHAR_BLANK_IDX);
        }
    }
#endif /* defined MAX7219SPI */
}

/* Construct and send the MIDI PC message */
static void sendMidiPC(uint16_t patch)
{
#ifdef USE_EXTERNAL_LED
    /* Flash the external LED to indicate data transfer */
    EXTERNAL_LED_ON(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN);
    ledTicks = LED_FLASH_LEN_MS;
#endif /* USE_EXTERNAL_LED */

    /* Send MIDI message */
    if (sendMIDIBank || range)
    {
        /* Send bank as a CC message -
           according to the spec CC 0 = MSB and CC 32 = LSB
           however Strymon docs suggest just unsing the MSB
           which also seems to work for a Boss ES-8 too.
           In contrast, the old ART X-15 foot controller
           uses both MSB and LSB which doesn't work for
           the ES-8 at least...
           So, just send the CC 0 message for the bank
           and revisit if necessary later */
        while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
            ;
        UART1_SendData8(MIDI_CC | midiChannel);
        while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
            ;
        UART1_SendData8(0x00);
        while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
            ;
        /* UART1_SendData8(0x00);

        while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
            ;
        UART1_SendData8(MIDI_CC | midiChannel);
        while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
            ;
        UART1_SendData8(0x20);
        while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
            ; */
        UART1_SendData8(patch / 128);
    }

    /* Send patch as PC message */
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
        ;
    UART1_SendData8(MIDI_PC | midiChannel);
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET)
        ;
    UART1_SendData8(patch % 128);

#ifdef RESTORELASTPC
    unlockEeprom();
    writeEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + LASTPCMSB, (patch >> 8) & 0xFF);
    writeEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + LASTPCLSB, patch & 0xFF);
    lockEeprom();
#endif /* RESTORELASTPC */

    displayPatch(patch);
}

/* Unlock EEPROM */
static void unlockEeprom(void)
{
    /* Define FLASH programming time */
    FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);

    /* Unlock Data memory */
    FLASH_Unlock(FLASH_MEMTYPE_DATA);

    /* Wait until Data EEPROM area unlocked flag is set */
    while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET)
        ;
}

/* Lock EEPROM */
static void lockEeprom(void)
{
    FLASH_Lock(FLASH_MEMTYPE_DATA);
}

/*  Write a byte to EEPROM */
static uint8_t writeEepromByte(uint32_t addr, uint8_t val)
{
    uint8_t chk;

    /* Write the byte */
    FLASH_ProgramByte(addr, val);

    /* Wait until End of Programming flag is set */
    while (FLASH_GetFlagStatus(FLASH_FLAG_EOP) == RESET)
        ;

    /* Read back and verify */
    chk = FLASH_ReadByte(addr);
    if (chk != val)
        return 1;
    return 0;
}

/* Read a byte from EEPROM */
static uint8_t readEepromByte(uint32_t addr)
{
    return FLASH_ReadByte(addr);
}

/* Get the MIDI channel, range and display mode from the EEPROM, reconfigure as required
   and if changed, update the EEPROM */
static void manageConfig(void)
{
    uint8_t buf;

    /* Load current MIDI channel, range and display mode from EEPROM */
    midiChannel = (readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + CHANNELOFFSET) & 0x0F);
    range = readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + RANGEOFFSET) % (MAXRANGE + 1);
    showZeroBased = readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + DISPLAYOFFSET) & 0x01;

    /* If a FS is held down on power-up, enter config mode otherwise return.
       The initial contents of the EEPROM is expected to be all zeros which will
       result in the device transmitting on channel 1 with a PC range of 0-127
       (displayed as 1 - 128) which should be a good default. */
    buf = scanFS(AUTOREPEAT_OFF, 100);
    if (buf == 0xFF)
        return;

    flashDisplay(START_FLASH);
    if (buf == DOWN)
    {
        configDisplay();
    }
    else
    {
        configMIDI();
    }
    flashDisplay(STOP_FLASH);

    /* EEPROM has finite life so only write if there was a change */
    if (midiChannel != (readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + CHANNELOFFSET) & 0x0F) ||
        range != readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + RANGEOFFSET) ||
        showZeroBased != readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + DISPLAYOFFSET))
    {
        /* Store values to EEPROM for next time */
        unlockEeprom();
        writeEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + CHANNELOFFSET, midiChannel);
        writeEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + RANGEOFFSET, range);
        writeEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + DISPLAYOFFSET, showZeroBased);
        lockEeprom();
    }
    return;
}

/* Select to display the actual PC value (eg 0 - 127) or more human friendly/common (eg 1 - 128) */
static void configDisplay(void)
{
    /* Show current value, 0 means 0 - 127 etc */
#if defined MAX7219SPI
    max7219_DisplayChar(1, showZeroBased ^ 1);
#elif defined SSD1306I2C
    ssd1306_DisplayChar(2, showZeroBased ^ 1);
#endif /* defined MAX7219SPI */

    while (1)
    {
        switch (scanFS(AUTOREPEAT_OFF, 3000))
        {
        case UP:
        case DOWN:
            showZeroBased ^= 1;
#if defined MAX7219SPI
            max7219_DisplayChar(1, showZeroBased ^ 1);
#elif defined SSD1306I2C
            ssd1306_DisplayChar(2, showZeroBased ^ 1);
#endif /* defined MAX7219SPI */
            break;

        default:
            return;
        }
    }
}

/* Select MIDI channel and the range of PC values */
static void configMIDI(void)
{
#if defined MAX7219SPI
    max7219_ShowMidiChannel(midiChannel, range);
#elif defined SSD1306I2C
    ssd1306_ShowMidiChannel(midiChannel, range);
#endif /* defined MAX7219SPI */

    while (1)
    {
        switch (scanFS(AUTOREPEAT_OFF, 3000))
        {
        case UP:
            /* Increment MIDI channel */
            midiChannel++;
            midiChannel %= 0x10;
#if defined MAX7219SPI
            max7219_ShowMidiChannel(midiChannel, range);
#elif defined SSD1306I2C
            ssd1306_ShowMidiChannel(midiChannel, range);
#endif /* defined MAX7219SPI */
            break;

        case DOWN:
            /* Decrement MIDI channel */
            midiChannel--;
            midiChannel %= 0x10;
#if defined MAX7219SPI
            max7219_ShowMidiChannel(midiChannel, range);
#elif defined SSD1306I2C
            ssd1306_ShowMidiChannel(midiChannel, range);
#endif /* defined MAX7219SPI */
            break;

        case MODE:
            /* Toggle extended range mode */
            range++;
            range %= (MAXRANGE + 1);
#if defined MAX7219SPI
            max7219_ShowMidiChannel(midiChannel, range);
#elif defined SSD1306I2C
            ssd1306_ShowMidiChannel(midiChannel, range);
#endif /* defined MAX7219SPI */
            break;

        default:
            /* Done */
            return;
        }
    }
}

/* Mode 2 - display flashes and increments/decrements on relevant footswitch press.
   If a switch is held down then the action autorepeats. Pressing the MODE switch
   sends the PC message, stops flashing and reverts to MODE 1. */
static void mode2(void)
{
    uint8_t i = 1;
    uint16_t newPatchNo;

    newPatchNo = midiPatchNo;

    /* Start flashing the display */
    flashDisplay(START_FLASH);

    while (i)
    {
        switch (scanFS(AUTOREPEAT_ON, 0))
        {
        case UP:
            newPatchNo++;
            newPatchNo %= (maxPatch[range] + 1);
            displayPatch(newPatchNo);
            break;

        case DOWN:
            newPatchNo--;
            if (newPatchNo > maxPatch[range])
            {
                newPatchNo = maxPatch[range];
            }
            displayPatch(newPatchNo);
            break;

        case MODE:
            i = 0;
        }
    }

    midiPatchNo = newPatchNo;
    sendMidiPC(midiPatchNo);

    /* Stop flashing the display */
    flashDisplay(STOP_FLASH);
}

/* Return the index of the FS pressed.

   if autoRepeat is AUTOREPEAT_ON then a held switch
   will fire again after AUTOREPEAT_AFTER_MS ms and after
   a while will decrease the autorepeat interval (faster baby).

   If timeoutMs is 0, the scan loop runs until a footswitch is
   pressed. If non 0, the value is the number of ms after
   which the function will return. If no footswitch was pressed,
   the returned value is 0xFF.
 */
static uint8_t scanFS(uint8_t autoRepeat, uint16_t timeoutMs)
{
    uint8_t i;
    uint16_t autoRepeatPeriod = AUTOREPEAT_AFTER_MS;
    uint32_t startScan;

    startScan = now;

    while (1)
    {
        if (timeoutMs > 0 && (now - startScan) > timeoutMs)
            return 0xFF;

        if (doFlash)
        {
#if defined MAX7219SPI
            max7219_DisplayIntensity(displayIntensity);
#elif defined SSD1306I2C
            ssd1306_DisplayIntensity(displayIntensity);
#endif /* defined MAX7219SPI */
        }

        for (i = 0; i < MAXFS; i++)
        {
            if ((GPIO_ReadInputData(FS_PORT) & fsArr[i].pin) == 0x00)
            {
                /* At least one switch is down */
#ifndef HAS_MODE_FS
                if (i == MODE)
                {
                    /* In the 2 switch version, one of the switches will be
                       pressed before the other, and since each switch actuation
                       state and time are stored persistently this means that the
                       button which was pressed first will cause a MIDI patchchange
                       to be sent a little before the mode change. This is undesireable
                       so if both switches are pressed the the state of the individual
                       switches is reset. */
                    fsArr[UP].state = FS_UP;
                    fsArr[DOWN].state = FS_UP;
                }
#endif /* !HAS_MODE_FS */
                switch (fsArr[i].state)
                {
                case FS_UP:
                    /* Freshly down */
                    fsArr[i].state = FS_DOWN;
                    fsArr[i].timeDown = now;
                    fsArr[i].firstDown = now;
                    break;

                case FS_DOWN:
                    /* Already down but not actioned */
                    if ((now - fsArr[i].timeDown) > DEBOUNCE_THRESHOLD_MS)
                    {
                        /* Down long enough, do it */
                        fsArr[i].state = FS_SENT;
                        fsArr[i].timeDown = now;
                        return i;
                    }
                    break;

                case FS_SENT:
                    if (autoRepeat == AUTOREPEAT_OFF)
                        continue;

                    /* Switch was actioned but is still down. After time t
                       return it again - autorepeat feature. This starts more
                       slowly then increases the rate of change. */
                    if ((now - fsArr[i].firstDown) > AUTOREPEAT_FAST_AFTER)
                    {
                        autoRepeatPeriod = AUTOREPEAT_FAST_MS;
                    }

                    if ((now - fsArr[i].timeDown) > autoRepeatPeriod)
                    {
                        /* Down long enough, do it */
                        fsArr[i].timeDown = now;
                        return i;
                    }
                }
            }
            else
            {
                /* Switch is up */
                fsArr[i].state = FS_UP;
                autoRepeatPeriod = AUTOREPEAT_AFTER_MS;
            }
        }
    }
}

void main(void)
{
    disableInterrupts();

    /* Initialise hardware */
    initClk();
    initTim2();
    initGpio();
    initUart();

    /* Enable interrupts so the timer is available */
    enableInterrupts();

#if defined MAX7219SPI
    max7219_Init();
#elif defined SSD1306I2C
    ssd1306_Init();
#endif /* defined MAX7219SPI */

    /* Get/set config */
    manageConfig();

    /* On startup, the MIDI program number is 0,
       display it but don't transmit it */
#ifdef RESTORELASTPC
    midiPatchNo = ((readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + LASTPCMSB) << 8) +
                   readEepromByte(FLASH_DATA_START_PHYSICAL_ADDRESS + LASTPCLSB));
#endif /* RESTORELASTPC */
    displayPatch(midiPatchNo);

    /* All initialisation is done */
#ifdef USE_EXTERNAL_LED
    /* Turn the external LED off (it flashes for MIDI TX */
    EXTERNAL_LED_OFF(LED_GPIO_PORT, (GPIO_Pin_TypeDef)LED_GPIO_PIN);
#endif /* USE_EXTERNAL_LED */

    /* Scan switches and send messages */
    while (1)
    {
        switch (scanFS(AUTOREPEAT_OFF, 0))
        {
        case UP:
            /* PC up */
            ++midiPatchNo;
            midiPatchNo %= (maxPatch[range] + 1);
            sendMidiPC(midiPatchNo);
            break;

        case DOWN:
            /* PC down */
            --midiPatchNo;
            if (midiPatchNo > maxPatch[range])
            {
                midiPatchNo = maxPatch[range];
            }
            sendMidiPC(midiPatchNo);
            break;

        case MODE:
            /* mode 2 */
            mode2();
            break;
        }
    }
}
