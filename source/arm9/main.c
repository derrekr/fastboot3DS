#include <stdio.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/firm.h"
#include "arm9/dev.h"
#include "arm9/fatfs/ff.h"
#include "hid.h"
#include "arm9/utils.h"
#include "arm9/linux.h"



void heap_init(void)
{
	extern void* fake_heap_start;
	extern void* fake_heap_end;
	fake_heap_start = (void*)A9_HEAP_START;
	fake_heap_end = (void*)A9_HEAP_END;
}

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

void testCode(void)
{
	FATFS nandFs;
	FRESULT res;

	printf("Mount res: %X\n", (res = f_mount(&nandFs, "twln:", 1)));
	if(res != FR_OK) return;
	FILINFO fileInfo;
	DIR dir;
	char lfn[255 + 1];
	fileInfo.lfname = lfn;
	fileInfo.lfsize = 256;

	f_opendir(&dir, "twln:/");
	while(f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != 0)
	{
		printf("%s 0x%X\n", ((*fileInfo.lfname) ? fileInfo.lfname : fileInfo.fname), (unsigned int)fileInfo.fsize);
	}
	f_closedir(&dir);
	printf("Unmount res: %X\n", f_mount(NULL, "twln:", 1));
}

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
		printf("\nA boot firm.bin\nB boot test_firm.bin\nY boot firm1:/\nUp enable 3dshax\nDown update loader\n");
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
				updateNandLoader("sdmc:/update.bin");
				/*if(!restoreNand())
				{
					continue_ = false;
					break;
				}*/
				//testCode();
				//loadLinux();
			}
			if(*((vu32*)CORE_SYNC_ID) == 2) // Poweroff signal from ARM11.
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
	}

	if(f_mount(NULL, "sdmc:", 1) != FR_OK)
	{
		initGfx();
		printf("Failed to unmount SD card fs!\n");
		return 1;
	}

	*((vu32*)CORE_SYNC_ID) = 0;
	if(!continue_) return 1;

	if(!firm_load_verify()) return 1;
	firm_launch(entry);

	return 0;
}
