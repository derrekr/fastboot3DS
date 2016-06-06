#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mem_map.h"
#include "console.h"
#include "dev.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "util_nand.h"



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

bool dumpNand(const char *filePath)
{
	const u32 nand_size = dev_rawnand->get_size();
	const u32 chunk_size = 0x80000;
	u32 cur_size;
	u32 cur_offset = 0;
	FIL file;
	FRESULT fres;
	UINT bytes_written;
	
	consoleSelect(&con_bottom);
	
	printf("\n\t\tPress B to cancel");
	
	consoleSelect(&con_top);
	
	u8 *buf = (u8 *) malloc(chunk_size);
	if(!buf)
	{
		printf("Not enough memory!\n");
		return false;
	}

	if(getFreeSpace("sdmc:") < nand_size)
	{
		printf("Not enough space on the SD card!\n");
		goto fail;
	}
	
	if((fres = f_open(&file, filePath, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
	{
		printf("Failed to create '%s'! Error: %x\n", filePath, fres);
		f_close(&file);
		goto fail;
	}

	while(cur_offset < nand_size)
	{
		if(cur_offset + chunk_size < nand_size) cur_size = chunk_size;
		else cur_size = nand_size - cur_offset;

		if(!dev_rawnand->read(cur_offset, cur_size, buf))
		{
			printf("\nFailed to read sector 0x%X!\n", (unsigned int)cur_offset>>9);
			f_close(&file);
			goto fail;
		}
		if((f_write(&file, buf, chunk_size, &bytes_written) != FR_OK) || (bytes_written != chunk_size))
		{
			printf("\nFailed to write to file!\n");
			f_close(&file);
			goto fail;
		}
		
		hidScanInput();
		
		if(hidKeysDown() & KEY_B)
		{
			printf("\n...canceled!\n");
			f_close(&file);
			goto fail;
		}
		
		cur_offset += cur_size;
		printf("\r%d/%d MB (Sector 0x%X/0x%X)", (unsigned int)cur_offset>>20, (unsigned int)nand_size>>20, 
				(unsigned int)cur_offset>>9, (unsigned int)nand_size>>9);
	}

	f_close(&file);
	free(buf);
	return true;
	
fail:
	free(buf);
	sleep_wait(0x8000000);
	return false;
}

bool restoreNand(const char *filePath)
{
	FIL file;
	UINT bytesRead;
	u32 cur_size;
	u32 cur_offset = 0;
	const u32 nand_size = dev_rawnand->get_size();
	const u32 chunk_size = 0x80000;
	u8 *buf;
	
	consoleSelect(&con_bottom);
	
	printf("\n\t\tPress B to cancel");
	
	consoleSelect(&con_top);
	
	buf = (u8 *) malloc(chunk_size);
	if(!buf)
	{
		printf("Not enough memory!\n");
		goto fail;
	}

	FILINFO fileStat;
	if(f_stat(filePath, &fileStat) != FR_OK)
	{
		printf("Failed to get file status!\n");
		goto fail;
	}
	if(fileStat.fsize > nand_size)
	{
		printf("NAND file is bigger than NAND!\n");
		goto fail;
	}
	if(fileStat.fsize < 0x200)
	{
		printf("NAND file is too small!\n");
		goto fail;
	}

	if(f_open(&file, filePath, FA_READ) != FR_OK)
	{
		printf("Failed to open '%s'!\n", filePath);
		f_close(&file);
		goto fail;
	}

	u32 size = fileStat.fsize;
	while(cur_offset < size)
	{
		if(cur_offset + chunk_size < nand_size) cur_size = chunk_size;
		else cur_size = nand_size - cur_offset;

		if(f_read(&file, buf, cur_size, &bytesRead) != FR_OK || bytesRead != cur_size)
		{
			printf("\nFailed to read from file!\n");
			f_close(&file);
			goto fail;
		}
		if(!dev_rawnand->write(cur_offset, cur_size, buf))
		{
			printf("\nFailed to write sector 0x%X!\n", (unsigned int)cur_offset>>9);
			f_close(&file);
			goto fail;
		}

		cur_offset += cur_size;
		printf("\r%d/%d MB (Sector 0x%X/0x%X)", (unsigned int)cur_offset>>20, (unsigned int)nand_size>>20, 
				(unsigned int)cur_offset>>9, (unsigned int)nand_size>>9);
	}

	f_close(&file);
	free(buf);
	return true;
	
fail:
	free(buf);
	sleep_wait(0x8000000);
	return false;
}
