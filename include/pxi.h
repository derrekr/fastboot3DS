
#pragma once

#define PXI_SYNC_CMD_RECEIVED(cmd)	(cmd | 0x1)
#define PXI_CMD_DUMMY			0
#define PXI_SYNC_CMD_CONNECT	0x40
#define PXI_SYNC_CMD_MMC_READ	0x42

#define PXI_RESULT_OK			0x4F4B4159

typedef struct
{
	vu8 cmd_id;
	vu32 cmd_params[8];
	vu32 result;
} _pxi_table;

static _pxi_table *pxi_table = (_pxi_table *) 0x1FFFFF80;

u8 pxi9_sync_getval()
{
	return PXI_SYNC9 & 0xFF;
}

void pxi9_sync_sendval(u8 val)
{
	u32 write_val = PXI_SYNC9;
	write_val &= 0xFFFF00FF;
	write_val |= val<<8;
	PXI_SYNC11 = write_val;
}

u8 pxi11_sync_getval()
{
	return PXI_SYNC11 & 0xFF;
}

void pxi11_sync_sendval(u8 val)
{
	u32 write_val = PXI_SYNC11;
	write_val &= 0xFFFF00FF;
	write_val |= val<<8;
	PXI_SYNC11 = write_val;
}
