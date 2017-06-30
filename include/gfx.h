#pragma once

#include "mem_map.h"
#include "types.h"


#define SCREEN_HEIGHT_TOP   (240)
#define SCREEN_WIDTH_TOP    (400)
#define SCREEN_SIZE_TOP     (SCREEN_HEIGHT_TOP * SCREEN_WIDTH_TOP * 2)
#define SCREEN_HEIGHT_SUB   (240)
#define SCREEN_WIDTH_SUB    (320)
#define SCREEN_SIZE_SUB     (SCREEN_HEIGHT_SUB * SCREEN_WIDTH_SUB * 2)

#define FRAMEBUF_TOP_A_1    (VRAM_BASE)
#define FRAMEBUF_SUB_A_1    (FRAMEBUF_TOP_A_1 + (SCREEN_WIDTH_TOP * SCREEN_HEIGHT_TOP * 2))
#define FRAMEBUF_TOP_A_2    (VRAM_BASE + 0x100000)
#define FRAMEBUF_SUB_A_2    (FRAMEBUF_TOP_A_2 + (SCREEN_WIDTH_TOP * SCREEN_HEIGHT_TOP * 2))

/// Converts packed RGB8 to packed RGB565.
#define RGB8_to_565(r,g,b)  (((b)>>3)&0x1f)|((((g)>>2)&0x3f)<<5)|((((r)>>3)&0x1f)<<11)


#ifdef ARM11
void gfx_clear_screens(u64 *framebufs1, u32 framebufs1Size, u64 *framebufs2, u32 framebufs2Size);
void GX_displayTransfer(u64 *in, u32 indim, u64 *out, u32 outdim, u32 flags);
void GX_textureCopy(u64 *in, u32 indim, u64 *out, u32 outdim, u32 size);
void gfx_swapFramebufs(void);
void gfx_init();
void gfx_deinit();
#endif
