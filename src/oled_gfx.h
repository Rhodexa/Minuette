#pragma once

#include <stdint.h>

// Drawing of pre-baked bitmaps — icons, big digits, animation frames — that
// live in flash. Same idea as oled_font.h's glyphs, but sized per-image
// instead of fixed at 5x7: each bitmap carries its own width and height.
//
// Bitmap bytes are stored "page-major": all `width` column-bytes of the
// bitmap's top page, then all `width` of the next page down, and so on —
// i.e. it's oledWritePage()'s own format, just concatenated for
// `heightPages` pages instead of one. So building one is: slice your image
// into 8px-tall horizontal bands and lay each band's column bytes out one
// after another, top band first.
//
// There's no built-in animation player — for a few-frame loop (e.g. a Wi-Fi
// icon), just keep an array of OledBitmap frames and call oledDrawBitmap()
// with whichever one is due, on whatever timer suits the caller.
struct OledBitmap {
  uint8_t width;       // pixels wide
  uint8_t heightPages; // pixels tall, in units of 8 (1 page = 8px)
  const uint8_t *data; // PROGMEM, width * heightPages bytes, page-major
};

// Draws `bmp` with its top-left corner at (page, col) — page in pages
// (0-7), col in pixels (0-127). Its bands land on consecutive pages
// (page, page+1, ...); placement is whole-page only, no sub-page (non-8px)
// vertical offsets. Clips at both panel edges.
void oledDrawBitmap(uint8_t page, uint8_t col, const OledBitmap &bmp);

void oledDrawSegments(uint8_t segments);
void oledDrawClock(int hour, int minute, bool has_colon);