#pragma once

#include "arm9/debug.h"
#include "fatfs/ff.h"
#include "arm9/ui.h"

typedef struct {
	char  model[0x10];
	bool  unit_is_new3ds;
	char  boot_env[0x20];
	char  mode[7];
	char  fw_ver1[10];
	char  fw_ver2[10];
	bool  wififlash_status;
	u32   nand_status;      // Bit 0: twln, bit 1: twlp, bit 2: nand
	u32   sd_status;        // 0: Not inserted, 1: FS init failed, 2: OK
} bootInfoStruct;

bootInfoStruct bootInfo;

void devs_close();
u8 rng_get_byte();
void power_off_safe();
void reboot_safe();
