/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "arm9/fatfs/ff.h"
#include "arm9/fatfs/diskio.h"		/* FatFs lower layer API */
#include "arm9/sdmmc.h"
#include "arm9/crypto.h"
#include "old_3DS_MBR_bin.h"



PARTITION VolToPart[] = {
    {0, 0},     /* Logical drive 0 ==> Physical drive 0, autodetect partitiion */
    {1, 1},     /* Logical drive 1 ==> Physical drive 1, 1st partition */
    {1, 2},     /* Logical drive 2 ==> Physical drive 1, 2nd partition */
    {1, 3}      /* Logical drive 3 ==> Physical drive 1, 3rd partition */
};

static u32 twlCounter[5];
static u32 ctrCounter[8];
static AES_ctx twlAesCtx, ctrAesCtx;


// TODO: Initialize keyslot 5 on N§DS!
void nandCryptInit(void)
{
	// Calc counters.
	sha((void*)0x01FFCD84, 16, twlCounter, SHA_INPUT_BIG | SHA_MODE_1, SHA_OUTPUT_BIG);
	sha((void*)0x01FFCD84, 16, ctrCounter, SHA_INPUT_BIG | SHA_MODE_256, SHA_OUTPUT_LITTLE);

	// Setup TWL NAND crypto keyslot.
	u32 keyX[4];
	u32 keyY[4];
	keyX[0] = (*((vu32*)0x01FFB808) ^ 0xB358A6AF) | 0x80000000;
	keyX[1] = 0x544E494E; // "NINT"
	keyX[2] = 0x4F444E45; // "ENDO"
	keyX[3] = *((vu32*)0x01FFB80C) ^ 0x08C267B7;
	AES_setTwlKeyX(AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER, 3, keyX);

	for(int i = 0; i < 3; i++) keyY[i] = ((vu32*)0x01FFD3C8)[i];
	keyY[3] = 0xE1A00005;
	AES_setTwlKeyY(AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER, 3, keyY);

	// Setup crypto params.
	AES_setCryptParams(&twlAesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_LITTLE |
						AES_INPUT_LITTLE | AES_OUTPUT_REVERSED_ORDER | AES_INPUT_REVERSED_ORDER | AES_MODE_CTR);
	AES_setCryptParams(&ctrAesCtx, AES_FLUSH_READ_FIFO | AES_FLUSH_WRITE_FIFO | AES_BIT12 | AES_BIT13 | AES_OUTPUT_BIG |
						AES_INPUT_BIG | AES_OUTPUT_NORMAL_ORDER | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR);
}

bool rwPartitionDec(u32 sector, u32 num, void *buf, bool write)
{
	bool isTwl = (sector<<9 < 0x0B0C8000);


	if(!write)
	{
		//printf("Sector read: %X, num: %X\n", (unsigned int)sector, (unsigned int)num);
		if(sdmmc_nand_readsectors(sector, num, buf))
		{
			printf("Failed to read sector 0x%X!\n", (unsigned int)sector);
			return false;
		}
	}

	if(isTwl)
	{
		AES_setCtrIvNonce(&twlAesCtx, twlCounter, AES_INPUT_LITTLE | AES_INPUT_REVERSED_ORDER | AES_MODE_CTR, sector<<9);

		if((sector >= 0x00012E00>>9 && sector < (0x00012E00 + 0x08FB5200)>>9) ||
			(sector >= 0x09011A00>>9 && sector < (0x09011A00 + 0x020B6600)>>9))
		{
			AES_selectKeyslot(3);
		}

		AES_crypt(&twlAesCtx, buf, buf, num<<9);
	}
	else
	{
		AES_setCtrIvNonce(&ctrAesCtx, ctrCounter, AES_INPUT_LITTLE | AES_INPUT_NORMAL_ORDER | AES_MODE_CTR, sector<<9);

		if(sector >= 0x0B100000>>9 && sector < (0x0B100000 + 0x00030000)>>9) // AGB savegame
		{
			AES_selectKeyslot(7);
		}
		else if(sector >= 0x0B130000>>9 && sector < (0x0B130000 + 0x00800000)>>9) // firm0:/ and firm1:/ together.
		{
			AES_selectKeyslot(6);
		}
		else if(sector >= 0x0B95CA00>>9 && sector < (0x0B95CA00 + 0x2F3E3600)>>9) // nand
		{
			// TODO: New 3DS support! On new 3DS keyslot 5 is used which uses a keyY initialized by
			// NATIVE_FIRM.
			AES_selectKeyslot(4);
		}

		AES_crypt(&ctrAesCtx, buf, buf, num<<9);
	}

	if(write)
	{
		//printf("Sector write: %X, num: %X\n", (unsigned int)sector, (unsigned int)num);
		if(sdmmc_nand_writesectors(sector, num, buf))
		{
			printf("Failed to write sector 0x%X!\n", (unsigned int)sector);
			return false;
		}
	}

	return true;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv == 1) nandCryptInit();
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if(pdrv == 0)
	{
		if(sdmmc_sdcard_readsectors(sector, count, buff)) return RES_ERROR;
	}
	else if(pdrv == 1)
	{
		if(!rwPartitionDec(sector, count, buff, false)) return RES_ERROR;
		if(sector == 0)
		{
			// TODO: Code to generate the MBR on-the-fly!
			memcpy(buff + 0x1B8, old_3DS_MBR_bin, old_3DS_MBR_bin_size);
		}
	}
	else return RES_ERROR;

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if(pdrv == 0)
	{
		if(sdmmc_sdcard_writesectors(sector, count, (u8*)buff)) return RES_ERROR;
	}
	else if(pdrv == 1)
	{
		if(!rwPartitionDec(sector, count, (u8*)buff, true)) return RES_ERROR;
	}
	else return RES_ERROR;

	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	return RES_OK;
}
