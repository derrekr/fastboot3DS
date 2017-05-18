#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arm9/nandimage.h"
#include "fatfs/ff.h"
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
