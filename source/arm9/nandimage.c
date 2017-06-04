#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arm9/dev.h"
#include "arm9/nandimage.h"
#include "util.h"

int validateNandImage(const char *filePath)
{
	FIL file;
	bool isContinuous;
	
	// only nand images on SD card are allowed.
	if(strncmp(filePath, "sdmc:", 5) != 0)
		return NANDIMG_ERROR_BADPATH;
	
	if(f_open(&file, filePath, FA_READ) != FR_OK)
		return NANDIMG_ERROR_NEXISTS;
	
	isContinuous = (file.obj.stat & 3) == 2;
	
	f_close(&file);
	
	if(!isContinuous)
		return NANDIMG_ERROR_NCONTS;
	
	return 0;
}

bool isNandImageCompatible(FIL *file)
{
	NCSD_header imageHeader;
	NCSD_header nandHeader;
	size_t bytesRead;
	
	if(f_read(file, &imageHeader, sizeof(NCSD_header), &bytesRead) != FR_OK
		|| bytesRead != sizeof(NCSD_header))
	{
		return false;
	}
	
	if(!dev_rawnand->read_sector(0, 1, &nandHeader))
	{
		return false;
	}
	
	/* compare everything except the signature */
	return memcmp(&imageHeader.magic, &nandHeader.magic, 0x100) == 0;
}
