#include <Arduino.h>
#include <Wire.h>
#include <string.h>

#include "oled.h"
#include "oled_font.h"
#include "config.h"

// SH1106 has 132 columns of RAM but only the middle 128 are wired to the
// panel glass; writes need to land at column 2 or the image shifts left.
// Some boards are wired for a different offset — if everything you draw
// comes out shifted sideways, this is the first thing to try changing.
static const uint8_t OLED_COL_OFFSET = 2;

// Data writes are split into chunks this size so a full 128-byte page
// write never risks overrunning Wire's internal transmit buffer.
static const uint8_t I2C_CHUNK = 32;

static void writeCommand(uint8_t cmd)
{
	Wire.beginTransmission(OLED_I2C_ADDR);
	Wire.write((uint8_t)0x00); // control byte: Co=0, D/C=0 (command stream)
	Wire.write(cmd);
	Wire.endTransmission();
}

static void setPageAndColumn(uint8_t page, uint8_t col)
{
	uint8_t ramCol = col + OLED_COL_OFFSET;
	writeCommand(0xB0 | (page & 0x0F));	  // set page address
	writeCommand(0x00 | (ramCol & 0x0F)); // set lower column nibble
	writeCommand(0x10 | (ramCol >> 4));	  // set higher column nibble
}

void oledInit()
{
	static const uint8_t initCmds[] = {
		0xAE, // display off
		0xD5,
		0x80, // display clock divide ratio / oscillator frequency
		0xA8,
		0x3F, // multiplex ratio = 64 (0x3F = 63)
		0xD3,
		0x00, // display offset = 0
		0x40, // display start line = 0
		0xAD,
		0x8B, // charge pump on (SH1106-specific)
		0xA1, // segment remap (mirror horizontal)
		0xC8, // COM scan direction (mirror vertical)
		0xDA,
		0x12, // COM pins hardware configuration
		0x81,
		0x80, // contrast, mid-range default
		0xD9,
		0x22, // pre-charge period
		0xDB,
		0x40, // VCOM deselect level
		0xA4, // resume RAM content display (not all-pixels-on)
		0xA6, // normal (not inverted)
	};
	for (uint8_t i = 0; i < sizeof(initCmds); i++)
	{
		writeCommand(initCmds[i]);
	}

	oledRawClear();
	writeCommand(0xAF); // display on
}

void oledRawClearPage(uint8_t page)
{
	static const uint8_t zeros[OLED_WIDTH_PX] = {0};
	oledWritePage(page, 0, zeros, OLED_WIDTH_PX);
}

void oledRawClear()
{
	for (uint8_t page = 0; page < OLED_HEIGHT_PAGES; page++)
	{
		oledRawClearPage(page);
	}
}

// Private to this file — a header-scope `static` array would give every
// .cpp that includes oled.h its own separate copy instead of one shared
// buffer, which is the bug this replaced.
static uint8_t oledBuffer[OLED_WIDTH_PX * OLED_HEIGHT_PAGES];

void oledClearBufferPage(uint8_t page)
{
	if (page >= OLED_HEIGHT_PAGES)
	{
		return;
	}
	memset(oledBuffer + (uint16_t)page * OLED_WIDTH_PX, 0x00, OLED_WIDTH_PX);
}

void oledClearBuffer()
{
	memset(oledBuffer, 0x00, sizeof(oledBuffer));
}

void oledBufWritePage(uint8_t page, uint8_t startCol, const uint8_t *columns, uint8_t count)
{
	if (page >= OLED_HEIGHT_PAGES || startCol >= OLED_WIDTH_PX)
	{
		return;
	}
	if ((uint16_t)startCol + count > OLED_WIDTH_PX)
	{
		count = OLED_WIDTH_PX - startCol; // clip rather than write past the row
	}
	memcpy(oledBuffer + (uint16_t)page * OLED_WIDTH_PX + startCol, columns, count);
}

void oledPushBuffer()
{
	for (uint8_t page = 0; page < OLED_HEIGHT_PAGES; page++)
	{
		oledWritePage(page, 0, oledBuffer + (uint16_t)page * OLED_WIDTH_PX, OLED_WIDTH_PX);
	}
}

void oledWritePage(uint8_t page, uint8_t startCol, const uint8_t *columns, uint8_t count)
{
	if (page >= OLED_HEIGHT_PAGES || startCol >= OLED_WIDTH_PX)
	{
		return;
	}
	if ((uint16_t)startCol + count > OLED_WIDTH_PX)
	{
		count = OLED_WIDTH_PX - startCol; // clip rather than write past the panel
	}

	setPageAndColumn(page, startCol);

	// SH1106's column pointer auto-increments after every data byte, so the
	// chunks below don't need to re-send the page/column address in between —
	// only the 0x40 control byte at the start of each I2C transaction.
	uint8_t sent = 0;
	while (sent < count)
	{
		uint8_t chunk = count - sent;
		if (chunk > I2C_CHUNK)
		{
			chunk = I2C_CHUNK;
		}
		Wire.beginTransmission(OLED_I2C_ADDR);
		Wire.write((uint8_t)0x40); // control byte: Co=0, D/C=1 (data stream)
		Wire.write(columns + sent, chunk);
		Wire.endTransmission();
		sent += chunk;
	}
}

void oledSetContrast(uint8_t value)
{
	writeCommand(0x81);
	writeCommand(value);
}

void oledSetPower(bool on)
{
	writeCommand(on ? 0xAF : 0xAE);
}

void oledSetInverted(bool inverted)
{
	writeCommand(inverted ? 0xA7 : 0xA6);
}

// Both oledPrintChar/oledRawPrintText (straight to the panel) and their
// oledBufPrintChar/oledPrintText counterparts (into the buffer) do the
// exact same glyph lookup — they only differ in which page-writer they
// hand the result to — so that's the one thing threaded through here as a
// function pointer, matching oledWritePage()/oledBufWritePage()'s signature.
typedef void (*PageWriter)(uint8_t page, uint8_t startCol, const uint8_t *columns, uint8_t count);

static uint8_t printCharVia(PageWriter writePage, uint8_t page, uint8_t col, char c)
{
	if (col >= OLED_WIDTH_PX)
	{
		return col;
	}

	uint8_t ch = (uint8_t)c;
	if (ch < OLED_FONT_FIRST_CHAR || ch > OLED_FONT_LAST_CHAR)
	{
		ch = '?';
	}
	const uint8_t *glyph = oledFont5x7[ch - OLED_FONT_FIRST_CHAR];

	// Glyph columns plus one blank spacer column between characters.
	uint8_t cell[OLED_FONT_GLYPH_WIDTH + 1];
	for (uint8_t i = 0; i < OLED_FONT_GLYPH_WIDTH; i++)
	{
		cell[i] = pgm_read_byte(&glyph[i]);
	}
	cell[OLED_FONT_GLYPH_WIDTH] = 0x00;

	uint8_t width = OLED_FONT_GLYPH_WIDTH + 1;
	writePage(page, col, cell, width); // writePage() clips at the right edge
	return col + width;
}

static uint8_t printTextVia(PageWriter writePage, uint8_t page, uint8_t col, const char *text)
{
	while (*text != '\0' && col < OLED_WIDTH_PX)
	{
		col = printCharVia(writePage, page, col, *text++);
	}
	return col;
}

uint8_t oledPrintChar(uint8_t page, uint8_t col, char c)
{
	return printCharVia(oledWritePage, page, col, c);
}

uint8_t oledRawPrintText(uint8_t page, uint8_t col, const char *text)
{
	return printTextVia(oledWritePage, page, col, text);
}

uint8_t oledBufPrintChar(uint8_t page, uint8_t col, char c)
{
	return printCharVia(oledBufWritePage, page, col, c);
}

uint8_t oledPrintText(uint8_t page, uint8_t col, const char *text)
{
	return printTextVia(oledBufWritePage, page, col, text);
}
