#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/dev.h"
#include "fatfs/ff.h"
#include "hid.h"
#include "util.h"
#include "arm9/firmwriter.h"
#include "arm9/main.h"
#include "arm9/menu.h"
#include "arm9/timer.h"

static size_t totalToWrite;
static size_t totalWritten;
static size_t curSector;
static bool protectSig;
static bool failure;

void firmwriterInit(size_t sector, size_t sectorCount, bool preserveSignature)
{
	if(sectorCount < 2)
		panic();

	totalToWrite = sectorCount;
	totalWritten = 0;
	// +1 because we'll write the firm header afterwards
	curSector = sector + 1;
	protectSig = preserveSignature;
	failure = false;
}

bool firmwriterWriteBlock()
{
	void *sourceBuf = (void *) FIRM_LOAD_ADDR + (totalWritten + 1) * 0x200;
	u32 blockData[0x80];

	if(totalWritten >= totalToWrite - 1)
		return false;

	if(!dev_rawnand->write_sector(curSector, 0x200, sourceBuf))
	{
		failure = true;
		return false;
	}

	/* read back data and compare */
	if(!dev_rawnand->read_sector(curSector, sizeof blockData, blockData) ||
		memcmp(sourceBuf, blockData, sizeof blockData) != 0)
	{
		failure = true;
		return false;
	}

	curSector++;
	totalWritten++;

	return true;
}

bool firmwriterIsDone()
{
     if(failure)
		return false;

	 return totalWritten == totalToWrite - 1;
}

bool firmwriterFinish()
{
	u32 blockData[0x80];
	firm_header header;
	void *firmBuf = (void *) FIRM_LOAD_ADDR;
	size_t sector = curSector - totalWritten - 1;

	/* read signature from NAND if we want to keep sighax */
	if(protectSig)
	{
		if(!dev_rawnand->read_sector(sector, sizeof header, &header))
		{
			failure = true;
			return false;
		}

		memcpy(&header, firmBuf, 0x100);
		firmBuf = (void *) &header;
	}

	if(!dev_rawnand->write_sector(sector, 0x200, firmBuf))
	{
		failure = true;
		return false;
	}

	/* read back data and compare */
	if(!dev_rawnand->read_sector(sector, sizeof blockData, blockData) ||
		memcmp(firmBuf, blockData, sizeof blockData) != 0)
	{
		failure = true;
		return false;
	}

	return true;
}
