#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arm9/dev.h"
#include "arm9/fsutils.h"
#include "util.h"

bool fsGetFreeSpaceOnDrive(const char *drive, u64 *freeSpace)
{
	FATFS *fs;
	DWORD freeClusters;

	if(f_getfree(drive, &freeClusters, &fs) != FR_OK)
	{
		return false;
	}
	
	*freeSpace = ((u64)(freeClusters * fs->csize)) * 512;
	
	return true;
}

bool fsEnsureMounted(const char *path)
{
	if(strncmp(path, "sdmc:", 5) == 0)
	{
		//if(f_chdrive("sdmc:") != FR_OK)
		{
			return f_mount(&sd_fs, "sdmc:", 1) == FR_OK;
		}
	}
	else if(strncmp(path, "nand:", 5) == 0)
	{
		//if(f_chdrive("nand:") != FR_OK)
		{
			return f_mount(&nand_fs, "nand:", 1) == FR_OK;
		}
	}
	
	return false;
}

void fsUnmountAll()
{
	f_mount(NULL, "sdmc:", 1);
	f_mount(NULL, "twln:", 1);
	f_mount(NULL, "twlp:", 1);
	f_mount(NULL, "nand:", 1);
}

u32 fsMountNandFilesystems()
{
	u32 res = 0;

	res |= (f_mount(&nand_twlnfs, "twln:", 1) ? 0 : 1u);
	res |= (f_mount(&nand_twlpfs, "twlp:", 1) ? 0 : 1u<<1);
	res |= (f_mount(&nand_fs, "nand:", 1) ? 0 : 1u<<2);

	return res;
}

void fsUnmountNandFilesystems()
{
	f_mount(NULL, "twln:", 1);
	f_mount(NULL, "twlp:", 1);
	f_mount(NULL, "nand:", 1);
	dev_decnand->close();
	dev_rawnand->close();
}

u32 fsRemountNandFilesystems()
{
	u32 res = 0;

	res |= (f_mount(&nand_twlnfs, "twln:", 1) ? 0 : 1u);
	res |= (f_mount(&nand_twlpfs, "twlp:", 1) ? 0 : 1u<<1);
	res |= (f_mount(&nand_fs, "nand:", 1) ? 0 : 1u<<2);

	return res;
}

bool fsMountSdmc()
{
	return f_mount(&sd_fs, "sdmc:", 1) == FR_OK;
}


