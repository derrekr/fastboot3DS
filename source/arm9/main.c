#include <stdio.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/console.h"
#include "arm9/dev.h"
#include "arm9/fatfs/ff.h"
#include "arm9/firm.h"
#include "arm9/sdmmc.h"
#include "crypto.h"
//#include "ndma.h"
#include "hid.h"
//#include "linux_config.h"



/*void test(void)
{
	u32 iv[4] = {0x9D38EAC5, 0xDD89ECED, 0x714D585A, 0xEB7DA30B};
	u32 key[4] = {0x0BB8CA27, 0x94C46C82, 0x5B31EA56, 0x1589DBD4};
	AES_ctx aesCtx;
	u8  buf[16] = {0};

	AES_setNormalKey(AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER, 4, key);
	AES_setCtrIvNonce(&aesCtx, iv, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CBC_ENCRYPT, 0);
	AES_setCryptParams(&aesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CBC_ENCRYPT);

	AES_crypt(&aesCtx, buf, buf, 16); // Result: 6C4C891B8A2CA7F8521A9DF6EE40538E Matches Openssl output.
	for(int i = 0; i < 16; i++) printf("%02X", buf[i]);
	printf("\n");
}*/

bool loadFirmNand(void)
{
	u32 hash[8];
	AES_ctx aesCtx;
	u32 size = 0;
	u8 *buf = (u8*)FIRM_LOAD_ADDR;


	// Hash the NAND CID the bootrom left for us in ITCM.
	sha((void*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);

	// Setup AES engine.
	AES_selectKeyslot(6); // firmX:/ keyslot
	AES_setCtrIvNonce(&aesCtx, hash, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, 0x0B530000);
	AES_setCryptParams(&aesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);

	// Read FIRM header from NAND and decrypt it.
	if(sdmmc_nand_readsectors(0x0B530000>>9, 1, buf))
	{
		printf("Failed to read FIRM header!\n");
		return false;
	}
	AES_crypt(&aesCtx, buf, buf, 512);
	buf += 512;

	// Calculate remaining size.
	firm_header *header = (firm_header*)FIRM_LOAD_ADDR;
	for(int i = 0; i < 4; i++) size += header->section[i].size;

	// Read remaining data.
	if(sdmmc_nand_readsectors(0x0B530200>>9, size>>9, buf))
	{
		printf("Failed to read FIRM sections!\n");
		return false;
	}

	// Decrypt remaining data of FIRM.
	AES_crypt(&aesCtx, buf, buf, size);

	return true;
}

bool loadFile(const char *filePath, void *address, u32 size, u32 *bytesRead)
{
	FIL file;
	bool res = true;


	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		return false;
	}
	if(f_read(&file, address, size, (UINT*)bytesRead) != FR_OK)
	{
		printf("Failed to read from file!\n");
		res = false;
	}
	f_close(&file);

	return res;
}

bool updateNandLoader(const char *filePath)
{
	u32 hash[8];
	AES_ctx aesCtx;
	u32 size, cmpVal = 0xAAAAAAAA;
	u8 *buf = (u8*)0x20800000;
	firm_header *header = (firm_header*)0x20800000;


	// Hash the NAND CID the bootrom left for us in ITCM.
	sha((void*)0x01FFCD84, 16, hash, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);

	// Setup AES engine.
	AES_selectKeyslot(6); // firmX:/ keyslot
	AES_setCtrIvNonce(&aesCtx, hash, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, 0x0B130000);
	AES_setCryptParams(&aesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);

	// Read FIRM from file.
	if(!loadFile(filePath, buf, FIRM_MAX_SIZE, &size)) return false;

	// Check for hax.
	if(memcmp(header->signature, &cmpVal, 4) == 0)
	{
		printf("Invalid FIRM!\n");
		return false;
	}

	// Delete update file.
	if(f_unlink(filePath) != FR_OK)
	{
		printf("Failed to delete '%s'!\n", filePath);
		return false;
	}

	// Encrypt FIRM.
	AES_crypt(&aesCtx, buf, buf, size);

	// Write FIRM to firm0:/.
	if(sdmmc_nand_writesectors(0x0B130000>>9, size>>9, buf))
	{
		printf("Failed to write sector 0xB130000!\n");
		return false;
	}

	printf("Loader successfully updated.\n");
	return true;
}

u64 getFreeSpace(const char *drive)
{
	FATFS *fs;
	DWORD freeClusters;

	if(f_getfree(drive, &freeClusters, &fs) != FR_OK)
	{
		printf("Failed to get free space for '%s'!\n", drive);
		return 0;
	}
	return ((u64)(freeClusters * fs->csize)) * 512;
}

bool dumpNand(void)
{
	u32 nandSectorCnt = getMMCDevice(0)->total_size;
	u32 sectorBlkSize = 0x4000, curSector = 0;
	u8 *buf = (u8*)0x20800000;
	FIL file;
	UINT bytesWritten;
	u32 blockSize;
	bool res = true;


	if(getFreeSpace("sdmc:") < nandSectorCnt<<9)
	{
		printf("Not enough space on the SD card!\n");
		return false;
	}
	if(f_open(&file, "sdmc:/NAND.bin", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		printf("Failed to create '%s'!\n", "sdmc:/NAND.bin");
		return false;
	}

	while(curSector < nandSectorCnt)
	{
		blockSize = ((nandSectorCnt - curSector < sectorBlkSize) ? nandSectorCnt - curSector : sectorBlkSize);

		if(sdmmc_nand_readsectors(curSector, blockSize, buf))
		{
			printf("\nFailed to read sector 0x%X!\n", (unsigned int)curSector);
			res = false;
			break;
		}
		if(f_write(&file, buf, blockSize<<9, &bytesWritten) != FR_OK || bytesWritten>>9 != blockSize)
		{
			printf("\nFailed to write to file!\n");
			res = false;
			break;
		}

		curSector += blockSize;
		printf("\r%u%% (Sector 0x%X/0x%X)", (unsigned int)(curSector * 100 / nandSectorCnt), (unsigned int)curSector, (unsigned int)nandSectorCnt);
	}

	f_close(&file);
	return res;
}

bool restoreNand(void)
{
	FIL file;
	UINT bytesRead;
	u32 blockSize;
	bool res = true;
	u32 offset = 0;
	u32 bufSize = 0x800000;
	u8 *buf = (u8*)0x20800000;


	FILINFO fileStat;
	if(f_stat("sdmc:/NAND.bin", &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		return false;
	}
	if(fileStat.fsize > (getMMCDevice(0)->total_size * 512))
	{
		printf("NAND file is bigger than NAND!\n");
		return false;
	}

	if(f_open(&file, "sdmc:/NAND.bin", FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", "sdmc:/NAND.bin");
		return false;
	}

	u32 size = fileStat.fsize;
	while(offset < size)
	{
		blockSize = ((size - offset < bufSize) ? size - offset : bufSize);

		if(f_read(&file, buf, blockSize, &bytesRead) != FR_OK || bytesRead != blockSize)
		{
			printf("\nFailed to read from file!\n");
			res = false;
			break;
		}
		if(sdmmc_nand_writesectors(offset>>9, blockSize>>9, buf))
		{
			printf("\nFailed to write sector 0x%X!\n", (unsigned int)offset>>9);
			res = false;
			break;
		}

		offset += blockSize;
		printf("\r%d%% (Sector 0x%X/0x%X)", (unsigned int)((u64)offset * 100 / size), (unsigned int)offset>>9, (unsigned int)size>>9);
	}

	f_close(&file);
	return res;
}

void initGfx(void)
{
	static bool isInitialized = false;

	if(!isInitialized)
	{
		isInitialized = true;
		consoleInit(1, NULL);
		CORE_SYNC_VAL = 1; // Tell ARM11 to turn on gfx.
		while(CORE_SYNC_VAL == 1);
	}
}

/*void loadLinux(void)
{
	u32 bytesRead;
	extern void linux_payloads_start;
	extern void linux_payloads_end;


	initGfx();
	loadFile(LINUXIMAGE_FILENAME, (void*)ZIMAGE_ADDR, 0x4000000, NULL);
	loadFile(DTB_FILENAME, (void*)PARAMS_TMP_ADDR, 0x400000, &bytesRead);
	f_mount(NULL, "sdmc:", 1);

	*((vu32*)0x214FFFFC) = bytesRead;

	NDMA_copy((void*)0x23F00000, &linux_payloads_start, ((u32)&linux_payloads_end - (u32)&linux_payloads_start)>>2);
	//NDMA_fill((u32*)0x18000000, 0, 0x00600000>>2);

	CORE_SYNC_VAL = 0x544F4F42;

	__asm__ __volatile__("ldr pc, =0x23F00000");
}*/

int main(void)
{
	FATFS fs;
	bool continue_ = true;
	void *entry = NULL;
	u32 kDown;


	hidScanInput();
	kDown = hidKeysDown();

	dev_sdcard->init();

	if(f_mount(&fs, "sdmc:", 1) != FR_OK)
	{
		initGfx();
		printf("Failed to mount SD card fs!\n");
		return 1;
	}

	if(kDown & KEY_R)
	{
		initGfx();
		printf("Boot3r v0.0a\n\n");
		printf("\nA boot firm.bin\nB boot test_firm.bin\nY boot firm1:/\nUp enable 3dshax\nDown test code\n");
		while(true)
		{
			hidScanInput();
			kDown = hidKeysDown();

			if(kDown & KEY_A)
			{
				if(!loadFile("sdmc:/firm.bin", (void*)FIRM_LOAD_ADDR, FIRM_MAX_SIZE, NULL)) continue_ = false;
				break;
			}
			if(kDown & KEY_B)
			{
				if(!loadFile("sdmc:/test_firm.bin", (void*)FIRM_LOAD_ADDR, FIRM_MAX_SIZE, NULL)) continue_ = false;
				break;
			}
			if(kDown & KEY_Y)
			{
				if(!loadFirmNand()) continue_ = false;
				break;
			}
			if(kDown & KEY_DUP)
			{
				entry = (void*)0x01FF8000;
				if(!loadFile("sdmc:/3dshax_arm9.bin", (void*)0x01FF8000, 0x8000, NULL)) continue_ = false;
				*((vu32*)0x01FF800C) = 3; // FIRMLAUNCH_RUNNINGTYPE = 3
				printf("3dshax enabled.\n");
			}
			if(kDown & KEY_DDOWN)
			{
				//updateNandLoader("sdmc:/update.bin");
				/*if(!restoreNand())
				{
					continue_ = false;
					break;
				}*/
				FATFS nandFs;

				printf("Mount res: %X\n", f_mount(&nandFs, "nand:", 1));
				FILINFO fileInfo;
				DIR dir;
				char lfn[255 + 1];
				fileInfo.lfname = lfn;
				fileInfo.lfsize = 256;

				f_opendir(&dir, "nand:/");
				while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != 0)
				{
					printf("%s 0x%X\n", ((*fileInfo.lfname) ? fileInfo.lfname : fileInfo.fname), (unsigned int)fileInfo.fsize);
				}
				f_closedir(&dir);
				printf("Unmount res: %X\n", f_mount(NULL, "nand:", 1));
			}
			if(CORE_SYNC_VAL == 2) // Poweroff signal from ARM11.
			{
				printf("Poweroff...\n");
				continue_ = false;
				break;
			}
		}
	}
	else
	{
		if(!loadFile("sdmc:/firm.bin", (void*)FIRM_LOAD_ADDR, FIRM_MAX_SIZE, NULL))
		{
			initGfx();
			printf("Failed to load '%s'!\n", "sdmc:/firm.bin");
			continue_ = false;
		}
		entry = (void*)0x01FF8000;
		if(!loadFile("sdmc:/3dshax_arm9.bin", (void*)0x01FF8000, 0x8000, NULL))
		{
			initGfx();
			printf("Failed to load '%s'!\n", "sdmc:/3dshax_arm9.bin");
			continue_ = false;
		}
		*((vu32*)0x01FF800C) = 3; // FIRMLAUNCH_RUNNINGTYPE = 3
		//loadLinux();
	}

	if(f_mount(NULL, "sdmc:", 1) != FR_OK)
	{
		initGfx();
		printf("Failed to unmount SD card fs!\n");
		return 1;
	}

	CORE_SYNC_VAL = 0;
	if(!continue_) return 1;

	if(!firm_load_verify()) return 1;
	firm_launch(entry);

	return 0;
}

void heap_init(void)
{
	extern void* fake_heap_start;
	extern void* fake_heap_end;
	fake_heap_start = (void*)A9_HEAP_START;
	fake_heap_end = (void*)A9_HEAP_END;
}
