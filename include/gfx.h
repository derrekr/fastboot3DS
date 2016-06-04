#pragma once

#define SCREEN_HEIGHT_TOP	240
#define SCREEN_WIDTH_TOP	400
#define SCREEN_HEIGHT_SUB	240
#define SCREEN_WIDTH_SUB	320

#define FRAMEBUF_TOP_A_1 0x18000000
#define FRAMEBUF_TOP_A_2 0x18000000
#define FRAMEBUF_SUB_A_1 0x1805DC00
#define FRAMEBUF_SUB_A_2 0x1805DC00


void gfx_init();
void gfx_set_framebufs(u8 r, u8 g, u8 b, u8 a);
void gfx_set_black_sub();
void gfx_draw_ppm();

inline u32 *gfx_get_framebuf_ptr(int top_screen, u16 x, u16 y)
{
	if(top_screen)
	{
		u32 *framebuf = (u32 *) FRAMEBUF_TOP_A_1;
		return &framebuf[SCREEN_HEIGHT_TOP*x + SCREEN_HEIGHT_TOP-y];
	}
	u32 *framebuf = (u32 *) FRAMEBUF_SUB_A_1;
	return &framebuf[SCREEN_HEIGHT_SUB*x + SCREEN_HEIGHT_SUB-y];
}
