#pragma once

#include "mem_map.h"
#include "types.h"


#define SCREEN_HEIGHT_TOP   (240)
#define SCREEN_WIDTH_TOP    (400)
#define SCREEN_HEIGHT_SUB   (240)
#define SCREEN_WIDTH_SUB    (320)

#define FRAMEBUF_TOP_A_1    (VRAM_BASE)
#define FRAMEBUF_TOP_A_2    (VRAM_BASE)
#define FRAMEBUF_SUB_A_1    (VRAM_BASE + (SCREEN_WIDTH_TOP * SCREEN_HEIGHT_TOP * 2))
#define FRAMEBUF_SUB_A_2    (VRAM_BASE + (SCREEN_WIDTH_TOP * SCREEN_HEIGHT_TOP * 2))

/// Converts packed RGB8 to packed RGB565.
#define RGB8_to_565(r,g,b)  (((b)>>3)&0x1f)|((((g)>>2)&0x3f)<<5)|((((r)>>3)&0x1f)<<11)


void gfx_clear_screens(u64 *top, u64 *sub);
void gfx_init();
void gfx_deinit();
