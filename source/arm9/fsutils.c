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

bool fsCreateFileWithPath(const char *filepath)
{
	const char *p = filepath;
	const char *subdir;
	const size_t maxBufSize = 0x1000;
	char *tempBuf = (char *) malloc(maxBufSize);
	char *oldPath = (char *) malloc(maxBufSize);
	FRESULT res;
	FIL tempFile;
	size_t copyLen;
	
	if(!tempBuf || !oldPath)
		return false;
	
	memset(oldPath, 0, maxBufSize);
	
	if(f_getcwd(oldPath, maxBufSize - 1) != FR_OK)
		goto fail_early;
	
	while(*p && *p != '/' && *p != '\\')
		p++;
		
	if(!*p)
		goto fail;
	
	p++;
	
	/* create directories */
	for(;;)
	{
		subdir = p;
		
		while(*p && *p != '/' && *p != '\\')
			p++;
		
		if(!*p) // not a directory or error
			break;
		
		copyLen = min((size_t)p - (size_t)subdir, maxBufSize - 1);
		memcpy(tempBuf, subdir, copyLen);
		tempBuf[copyLen] = '\0';
		
		res = f_mkdir(tempBuf);
		if(res != FR_OK && res != FR_EXIST)
			goto fail;
		
		if(f_chdir(tempBuf) != FR_OK)
			goto fail;
		
		p++;
	}
	
	if(p == subdir + 1)
		goto fail;
	
	/* touch file */
	res = f_open(&tempFile, filepath, FA_CREATE_ALWAYS);
	if(res != FR_OK && res != FR_EXIST)
		goto fail;
	
	f_close(&tempFile);
	
	
	if(f_chdir(oldPath) != FR_OK)
		goto fail;
	
	free(tempBuf);
	free(oldPath);
	
	return true;
	
fail:
	
	// This should never fail
	f_chdir(oldPath);
	
fail_early:

	free(tempBuf);
	free(oldPath);
	
	return false;
}

