#include <Arduino.h>

#include "oled_gfx.h"
#include "oled.h"

void oledDrawBitmap(uint8_t page, uint8_t col, const OledBitmap &bmp)
{
	uint8_t width = bmp.width;
	if (width > OLED_WIDTH_PX)
	{
		width = OLED_WIDTH_PX; // guard the stack copy below against a malformed bitmap
	}

	uint8_t band[OLED_WIDTH_PX];
	for (uint8_t i = 0; i < bmp.heightPages; i++)
	{
		uint8_t destPage = page + i;
		if (destPage >= OLED_HEIGHT_PAGES)
		{
			break; // ran off the bottom of the panel
		}
		memcpy_P(band, bmp.data + (uint16_t)i * bmp.width, width);
		oledBufWritePage(destPage, col, band, width); // oledBufWritePage() clips at the right edge
	}
}


/*
  Curde but works!
*/

static uint8_t oled_segment_rounded_top_left[] = {
	0xE0, 0xF8, 0xFC, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF
};
static uint8_t oled_segment_rounded_top_right[] = {
	0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFC, 0xF8, 0xE0
};
static uint8_t oled_segment_rounded_bottom_left[] = {
	0x07, 0x1F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF
};
static uint8_t oled_segment_rounded_bottom_right[] = {
	0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x1F, 0x07
};
static uint8_t oled_segment_block[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static uint8_t oled_segment_samll_block[] = {
	0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00
};
static uint8_t oled_segment_black_block[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int segment_col = 0;
static int segment_row = 0;

void oledDrawSegments(uint8_t segments) {
	int col = segment_col;
	int row = segment_row;

	if(segments & 0b10000000) {
		oledBufWritePage(row, col, oled_segment_rounded_top_left, 8);
		oledBufWritePage(row, col + 8, oled_segment_block, 8);
		oledBufWritePage(row + 1, col + 8, oled_segment_block, 8);
		oledBufWritePage(row + 2, col + 8, oled_segment_block, 8);
		oledBufWritePage(row + 3, col + 8, oled_segment_block, 8);
		oledBufWritePage(row + 4, col + 8, oled_segment_block, 8);
		return;
	}

	// main segments
	if(segments & 0b0000001) oledBufWritePage(row + 0, col + 8*1, oled_segment_block, 8);
	if(segments & 0b0000010) oledBufWritePage(row + 1, col + 8*2, oled_segment_block, 8);
	if(segments & 0b0000100) oledBufWritePage(row + 3, col + 8*2, oled_segment_block, 8);
	if(segments & 0b0001000) oledBufWritePage(row + 4, col + 8*1, oled_segment_block, 8);
	if(segments & 0b0010000) oledBufWritePage(row + 3, col + 8*0, oled_segment_block, 8);
	if(segments & 0b0100000) oledBufWritePage(row + 1, col + 8*0, oled_segment_block, 8);
	if(segments & 0b1000000) oledBufWritePage(row + 2, col + 8*1, oled_segment_block, 8);

	// corners
	row = segment_row;
	col = segment_col;
	if(segments & 0b1000001) oledBufWritePage(row, col, oled_segment_rounded_top_left, 8);
	if(segments & 0b0000011) oledBufWritePage(row, col + 16, oled_segment_rounded_top_right, 8);
	if(segments & 0b0011000) oledBufWritePage(row + 4, col, oled_segment_rounded_bottom_left, 8);
	if(segments & 0b0001100) oledBufWritePage(row + 4, col + 16, oled_segment_rounded_bottom_right, 8);

	// middle ones

	row = segment_row + 2;
	col = segment_col;
	if((segments & 0b0110000) == 0b0110000) {
		oledBufWritePage(row, col, oled_segment_block, 8);
	}
	else {
		if(segments & 0b0010000) oledBufWritePage(row, col, oled_segment_rounded_top_left, 8);
		if(segments & 0b0100000) oledBufWritePage(row, col, oled_segment_rounded_bottom_left, 8);
	}

	col = segment_col + 8*2;
	if((segments & 0b0000110) == 0b0000110) {
		oledBufWritePage(row, col, oled_segment_block, 8);
	}
	else {
		if(segments & 0b0000100) oledBufWritePage(row, col, oled_segment_rounded_top_right, 8);
		if(segments & 0b0000010) oledBufWritePage(row, col, oled_segment_rounded_bottom_right, 8);
	}
}

uint8_t segment_numbers[] = {
	0b0111111,
	0b10000000, // hacky special case for 1! (just for now) (or is it? this could draw fun things lol)
	0b1011011,
	0b1001111,
	0b1100110,
	0b1101101,
	0b1111101,
	0b0000111,
	0b1111111,
	0b1101111,
	0b1110111,
	0b1111100,
	0b1011000,
	0b1011110,
	0b1111001,
	0b1110001
};

void oledDrawClock(int hour, int minute, bool has_colon)
{
	oledClearBufferPage(1);
	oledClearBufferPage(2);
	oledClearBufferPage(3);
	oledClearBufferPage(4);
	oledClearBufferPage(5);
	segment_col = 3;
	segment_row = 1;
	oledDrawSegments(segment_numbers[hour / 10]);
	
	segment_col += 7*4;
	oledDrawSegments(segment_numbers[hour % 10]);
	
	segment_col += 7*6;
	oledDrawSegments(segment_numbers[minute / 10]);
	
	segment_col += 7*4;
	oledDrawSegments(segment_numbers[minute % 10]);

	if(has_colon) {
		oledBufWritePage(2, 60, oled_segment_block, 8);
		oledBufWritePage(4, 60, oled_segment_block, 8);
	}
	else {
		oledBufWritePage(2, 60, oled_segment_samll_block, 8);
		oledBufWritePage(4, 60, oled_segment_samll_block, 8);
	}
}
