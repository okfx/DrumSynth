#pragma once

// ============================================================================
//  OLED Bitmap Icons (1-bit, const)
//  Used by the display routines for transport state indicators.
//  Format: packed horizontal bits, MSB-first, stored in flash (ARM: const = flash).
// ============================================================================

// Stop icon — inset square (10x10 px)
//  ..........
//  .########.
//  .########.
//  .########.
//  .########.
//  .########.
//  .########.
//  .########.
//  .########.
//  ..........
static const unsigned char image_stop_bits[] = {0x00,0x00,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x7f,0x80,0x00,0x00};

// Play icon — inset right-pointing triangle (10x10 px)
//  ..........
//  .##.......
//  .####.....
//  .######...
//  .########.
//  .########.
//  .######...
//  .####.....
//  .##.......
//  ..........
static const unsigned char image_play_bits[] = {0x00,0x00,0x60,0x00,0x78,0x00,0x7e,0x00,0x7f,0x80,0x7f,0x80,0x7e,0x00,0x78,0x00,0x60,0x00,0x00,0x00};

