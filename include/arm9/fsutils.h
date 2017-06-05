#pragma once

#include "types.h"
#include "fatfs/ff.h"

// SD card FAT fs instance
FATFS sd_fs;
// same for all NAND filesystems
FATFS nand_twlnfs, nand_twlpfs, nand_fs;

bool fsGetFreeSpaceOnDrive(const char *drive, u64 *freeSpace);
bool fsEnsureMounted(const char *path);
void fsUnmountAll();
u32 fsMountNandFilesystems();
void fsUnmountNandFilesystems();
u32 fsRemountNandFilesystems();
bool fsMountSdmc();
