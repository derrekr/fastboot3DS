
#pragma once


#define SPIFLASH_CMD_RDSR   0x05
#define SPIFLASH_CMD_READ   0x03

// true if spiflash is installed, false otherwise
bool spiflash_get_status();
void spiflash_read(u32 offset, u32 size, u8 *buf);
