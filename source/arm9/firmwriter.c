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
static void *blockData = NULL;
static bool protectSig;
static bool failure;

/* sector: start sector								*/
/* blockCount: firmware total length in sectors		*/
/* preserveSignature: flag for sighax protection	*/
bool firmwriterInit(size_t sector, size_t blockCount, bool preserveSignature)
{
	if(blockCount < FIRMWRITER_SECTORS_PER_BLOCK * 2)
		return false;

	totalToWrite = blockCount;
	totalWritten = 0;
	// + 1 block because we'll write the firm header afterwards
	curSector = sector + FIRMWRITER_SECTORS_PER_BLOCK;
	protectSig = preserveSignature;
	failure = false;

	if(blockData)
	{
		free(blockData);
	}

	blockData = malloc(FIRMWRITER_SECTORS_PER_BLOCK * 0x200);
	
	return blockData != NULL;
}

/* returns number of sectors written, 0 == failure */
size_t firmwriterWriteBlock()
{
	size_t numSectors;
	
	void *sourceBuf = (void *) FIRM_LOAD_ADDR + (totalWritten + FIRMWRITER_SECTORS_PER_BLOCK) * 0x200;

	if(totalWritten >= totalToWrite - FIRMWRITER_SECTORS_PER_BLOCK)
		return 0;
		
	numSectors = min(FIRMWRITER_SECTORS_PER_BLOCK, totalToWrite - FIRMWRITER_SECTORS_PER_BLOCK - totalWritten);

	if(!dev_decnand->write_sector(curSector, numSectors, sourceBuf))
	{
		failure = true;
		return 0;
	}

	/* read back data and compare */
	if(!dev_decnand->read_sector(curSector, numSectors, blockData) ||
		memcmp(sourceBuf, blockData, numSectors * 0x200) != 0)
	{
		failure = true;
		return 0;
	}

	curSector += numSectors;
	totalWritten += numSectors;

	return numSectors;
}

bool firmwriterIsDone()
{
     if(failure)
		return false;

	 return totalWritten >= totalToWrite - FIRMWRITER_SECTORS_PER_BLOCK;
}

/* returns number of sectors written, 0 == failure */
size_t firmwriterFinish()
{
	static firm_header header;
	void *firmBuf = (void *) FIRM_LOAD_ADDR;
	void *compBuf = blockData;
	size_t sector = curSector - totalWritten - FIRMWRITER_SECTORS_PER_BLOCK;
	
	if(!firmwriterIsDone())
	{
		printf("firmwriter is not done yet.\n");
		return false;
	}

	/* read signature from NAND if we want to keep sighax */
	if(protectSig)
	{
		memcpy(blockData, firmBuf, FIRMWRITER_SECTORS_PER_BLOCK * 0x200);
		
		if(!dev_decnand->read_sector(sector, 1, &header))
		{
			failure = true;
			return 0;
		}

		memcpy(blockData + 0x100, &header.signature, 0x100);
		firmBuf = blockData;
		compBuf = malloc(FIRMWRITER_SECTORS_PER_BLOCK * 0x200);
		if(!compBuf)
		{
			failure = true;
			return 0;
		}
	}

	if(!dev_decnand->write_sector(sector, FIRMWRITER_SECTORS_PER_BLOCK, firmBuf))
	{
		failure = true;
		return 0;
	}

	/* read back data and compare */
	if(!dev_decnand->read_sector(sector, FIRMWRITER_SECTORS_PER_BLOCK, compBuf) ||
		memcmp(firmBuf, compBuf, FIRMWRITER_SECTORS_PER_BLOCK * 0x200) != 0)
	{
		failure = true;
		return 0;
	}

	if(compBuf != blockData)
		free(compBuf);

	if(blockData)
	{
		free(blockData);
		blockData = NULL;
	}

	return FIRMWRITER_SECTORS_PER_BLOCK;
}
