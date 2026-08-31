#pragma once

#include <stdint.h>

static const uint8_t OLED_WIDTH_PX = 128;
static const uint8_t OLED_HEIGHT_PAGES = 8; // 8 pages * 8px = 64px tall


// Minimal driver for a 128x64 SH1106-based I2C OLED.

// The write primitive mirrors the controller's own memory layout instead of
// wrapping it in a framebuffer: the panel is 8 "pages" stacked top to
// bottom, each one 8 pixels tall and 128 pixels wide, and each byte you send
// is one vertical column within a page — bit 0 is that column's top pixel,
// bit 7 its bottom pixel. A 5x7 font glyph or a hand-drawn icon in that
// shape can be sent as-is.

void oledInit();

// Fills every page with 0x00 (blank).
void oledRawClear();

// Fills one page (0-7) with 0x00.
void oledRawClearPage(uint8_t page);

// Writes `count` column bytes into `page` (0-7), starting at `startCol`
// (0-127), in SH1106's native page/column format described above. Goes
// straight to the panel over I2C — for a one-off write (a line of text,
// a small icon) this is simpler than the buffer below, since there's
// nothing to flush.
void oledWritePage(uint8_t page, uint8_t startCol, const uint8_t *columns, uint8_t count);

// --- Off-screen buffer ---
//
// A second, RAM-side copy of the panel's 1024 bytes (128 cols x 8 pages),
// laid out exactly like the panel's own page/column format. Draw into it
// with oledBufWritePage()/oledClearBuffer() — same shapes as the functions
// above, just landing in RAM instead of going out over I2C — then call
// oledPushBuffer() once to send the whole thing to the panel in one shot.
//
// Worth it when you're building up several pieces of a screen (icon + big
// numbers + text) and don't want each piece to flash into view separately
// as it's drawn — draw everything into the buffer first, then flush once.

// Fills the whole buffer with 0x00 (blank). Doesn't touch the panel.
void oledClearBuffer();

// Fills one page (0-7) of the buffer with 0x00. Doesn't touch the panel.
void oledClearBufferPage(uint8_t page);

// Same as oledWritePage(), but writes into the buffer instead of the panel.
void oledBufWritePage(uint8_t page, uint8_t startCol, const uint8_t *columns, uint8_t count);

// Sends the entire buffer to the panel, page by page.
void oledPushBuffer();

// 0 (dim) - 255 (brightest). Panel default is roughly mid-range.
void oledSetContrast(uint8_t value);

// Puts the panel to sleep (RAM contents are preserved) or wakes it back up.
void oledSetPower(bool on);

// Swaps which bit value is "lit" — a cheap way to flash/highlight the whole
// screen without touching its contents.
void oledSetInverted(bool inverted);

// --- Text ---
//
// Fixed-width 5x7 font, printable ASCII only (0x20-0x7E; anything outside
// that range prints as '?'). One character is 6 columns wide (5 for the
// glyph + 1 blank spacer column), so a 128px-wide page fits 21 characters.
//
// Each comes in a straight-to-panel flavor and a Buf flavor, same as the
// page writers above — use the Buf ones when the text is part of a screen
// you're building up in the buffer before one oledPushBuffer().

// Draws one character at (page, col) and returns the column just past it
// (col + 6), so calls can be chained to lay out text piece by piece.
uint8_t oledPrintChar(uint8_t page, uint8_t col, char c);
uint8_t oledBufPrintChar(uint8_t page, uint8_t col, char c);

// Draws a whole string starting at (page, col), stopping early if it runs
// off the right edge of the panel. Returns the column just past the last
// character drawn.
uint8_t oledRawPrintText(uint8_t page, uint8_t col, const char *text);
uint8_t oledPrintText(uint8_t page, uint8_t col, const char *text);
