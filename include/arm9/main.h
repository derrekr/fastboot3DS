#pragma once

// Shut up gcc warning
/*typedef struct config_format {
	u8 dev_id;

};*/

// PrintConsole for each screen
extern PrintConsole con_top, con_bottom;
// SD card FAT fs instance
extern FATFS sd_fs;
// same for all NAND filesystems
extern FATFS nand_twlnfs, nand_twlpfs, nand_fs;

extern bool	unit_is_new3ds;
extern u8	boot_env;


void heap_init();
void devs_init();
void devs_close();
void mount_fs();
void unmount_fs();
void unit_detect();
u8 rng_get_byte();
