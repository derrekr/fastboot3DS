#pragma once

#include "fatfs/ff.h"
#include "arm9/ui.h"

typedef struct {
	char	model[0x10];
	bool	unit_is_new3ds;
	char	boot_env[0x20];
	char	mode[7];
	char	fw_ver1[10];
	char	fw_ver2[10];
	bool	wififlash_status;
	bool	nand_status;
	bool	sd_status;
} bootInfoStruct;

bootInfoStruct bootInfo;

// SD card FAT fs instance
FATFS sd_fs;

// same for all NAND filesystems
FATFS nand_twlnfs, nand_twlpfs, nand_fs;

void devs_close();
bool remount_nand_fs();
void unmount_fs();
void unmount_nand_fs();
u8 rng_get_byte();
void power_off_safe();
