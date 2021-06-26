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
 
/**
   Minimalist font very loosly based on 7 segment display. Main aim was a simple,
   clean and small (in terms of bytes) font. This should be read in conjunction with
   the SSD1306 datasheet to better understand how it works.

   Each character is 32x56 pixels. The SSD1306 arranges the display into 8 PAGEs
   where each PAGE is 8 bits deep. This font is 7 PAGEs vertically. Since space
   is a premium, the assumption was made that the 1st, 2nd, 4th, 5th and 7th lines
   may differ but 2nd and 3rd are the same and 5th and 6th are the same hence the
   data for a character may be represented by 5 'lines'. Further, each of these
   lines can be represented by 3 bytes - the first used for bytes 0 and 1, the
   second for the next 28 bytes and the third for the last two bytes.

   The font_map[] array uses 5 bytes per character where each byte is an index
   into the font_lines{] array which encodes for one of the 7 pages of data.

   To display a character, the index of that character in the font_map{} array
   is determined. The first byte at that location is then used as the index in
   the font_lines[] array. The first byte is  written to the 1st two bytes of
   the SSD1306 PAGE0, the next byte is written to the next 28 bytes and finally
   the last of the 3 bytes is written to the last two bytes of the PAGE,
   forming 1 of the 7 lines of the character on screen.

   The next byte in font_map[] is then used to index into font_lines[] and the
   process repeated twice for PAGE1 and PAGE2. This process is repeated with
   the 3rd font_map[] byte indexing into font_lines[] for PAGE3, the 4th for
   PAGE4 and PAGE5 and the 5th byte for PAGE6. Hence worst case a character may
   be represented with 20 bytes.
*/

#ifndef FONT_H_
#define FONT_H_

/* Constants -----------------------------------------------------------------*/

/* Font definition */
/* simple font comprised of straight lines - minimal size.
   first define the various 'line' types. Assume patterns of
   2 repeated bytes + 28 repeated bytes + 2 repeated bytes
   then each line is represented by 3 bytes. */
const uint8_t font_lines[] =
{
    0xff, 0x03, 0xff, /* 0 */
    0xff, 0x00, 0xff, /* 1 */
    0xff, 0xc0, 0xff, /* 2 */
    0x00, 0x00, 0xff, /* 3 */
    0x03, 0x03, 0xff, /* 4 */
    0x00, 0x00, 0xff, /* 5 */
    0xff, 0xc0, 0xc0, /* 6 */
    0xc0, 0xc0, 0xff, /* 7 */
    0xff, 0x03, 0x03, /* 8 */
    0xff, 0x00, 0x00, /* 9 */
    0x00, 0x00, 0x00, /* 10 */
    0xF8, 0x18, 0x1F, /* 11 */
    0x18, 0x18, 0xFF, /* 12 */
    0x1F, 0x18, 0xff, /* 13 */
    0x1f, 0x18, 0xf8, /* 14 */
    0xff, 0x18, 0xff, /* 15 */
    0xff, 0xc0, 0xff, /* 16 */
    0xf8, 0x18, 0x18, /* 17 */
    0xf8, 0x00, 0xf8, /* 18 */
    0x1f, 0x18, 0x18, /* 19 */
    0xff, 0x18, 0xf8, /* 20 */
};

    
const uint8_t font_map[] =
{
    0, 1, 1, 1, 2,     /* 0 */
    3, 3, 3, 3, 3,     /* 1 */
    4, 3, 11, 9, 6,    /* 2 */
    4, 5, 12, 5, 7,    /* 3 */
    1, 1, 13, 3, 3,    /* 4 */
    8, 9, 14, 3, 7,    /* 5 */
    8, 9, 20, 1, 2,    /* 6 */
    4, 3, 3, 3, 3,     /* 7 */
    0, 1, 15, 1, 2,    /* 8 */
    0, 1, 13, 3, 7,    /* 9 */

    10, 10, 17, 9, 6,  /* c */
    8, 9, 19, 10, 10,  /* cc */
    8, 9, 9, 9, 6,     /* C */
    10, 10, 18, 1, 16, /* u */
    1, 1, 1, 1, 16,    /* U */
    10, 10, 10, 10, 10,/* blank */
};

#endif /* FONT_H_ */
