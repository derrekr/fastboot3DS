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
static size_t blockSize;
static void *blockData;
static bool protectSig;
static bool failure;

/* sector: start sector								*/
/* length: firmware total length in bytes			*/
/* block_Size: size of one block in bytes			*/
/* preserveSignature: flag for sighax protection	*/
void firmwriterInit(size_t sector, size_t length, size_t block_Size, bool preserveSignature)
{
	if(!block_Size || block_Count * block_Size < 2 * 0x200)
		return false;

	if(block_Count > UINT_MAX / block_Size)
		return false;

	if(sector % block_Size)
		return false;

	totalToWrite = block_Count;
	totalWritten = 0;
	blockSize = block_Size;
	// + 1 block because we'll write the firm header afterwards
	curSector = sector + block_Size / 0x200;
	protectSig = preserveSignature;
	failure = false;

	if(blockData)
	{
		free(blockData);
	}

	blockData = malloc(block_Size * block_Count);
	
	return blockData != NULL;
}

bool firmwriterWriteBlock()
{
	void *sourceBuf = (void *) FIRM_LOAD_ADDR + (totalWritten + 1) * 0x200;

	if(totalWritten >= totalToWrite - 1)
		return false;

	if(!dev_decnand->write_sector(curSector, 1, sourceBuf))
	{
		failure = true;
		return false;
	}

	/* read back data and compare */
	if(!dev_decnand->read_sector(curSector, 1, blockData) ||
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
		if(!dev_decnand->read_sector(sector, 1, &header))
		{
			failure = true;
			return false;
		}

		memcpy(&header, firmBuf, 0x100);
		firmBuf = (void *) &header;
	}

	if(!dev_decnand->write_sector(sector, 1, firmBuf))
	{
		failure = true;
		return false;
	}

	/* read back data and compare */
	if(!dev_decnand->read_sector(sector, 1, blockData) ||
		memcmp(firmBuf, blockData, sizeof blockData) != 0)
	{
		failure = true;
		return false;
	}

	if(blockData)
	{
		free(blockData);
		blockData = NULL;
	}

	return true;
}
