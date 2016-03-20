/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/*-----------------------------------------------------------------------*/

#include <stdbool.h>
#include "fatfs/ff.h"
#include "fatfs/diskio.h"		/* FatFs lower layer API */
#include "types.h"
#include "dev.h"



// Get's set externally in dev.c
u32 ctr_nand_offset;

PARTITION VolToPart[] = {
    {0, 0},     /* Logical drive 0 ==> Physical drive 0, autodetect partitiion */
    {1, 1},     /* Logical drive 1 ==> Physical drive 1, 1st partition */
    {1, 2},     /* Logical drive 2 ==> Physical drive 1, 2nd partition */
    {2, 1}      /* Logical drive 3 ==> Physical drive 2, 1st partition */
};

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
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
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
	if(pdrv == FATFS_DEV_NUM_SD)
	{
		if(dev_sdcard->read((u32)sector<<9, (u32)count<<9, buff)) return RES_OK;
	}
	else if(pdrv == FATFS_DEV_NUM_TWL_NAND)
	{
		if(dev_decnand->read((u32)sector<<9, (u32)count<<9, buff)) return RES_OK;
	}
	else if(pdrv == FATFS_DEV_NUM_CTR_NAND)
	{
		if(dev_decnand->read(ctr_nand_offset + ((u32)sector<<9), (u32)count<<9, buff)) return RES_OK;
	}

	return RES_ERROR;
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
	if(pdrv == FATFS_DEV_NUM_SD)
	{
		if(dev_sdcard->write((u32)sector<<9, (u32)count<<9, buff)) return RES_OK;
	}
	else if(pdrv == FATFS_DEV_NUM_TWL_NAND)
	{
		if(dev_decnand->write((u32)sector<<9, (u32)count<<9, buff)) return RES_OK;
	}
	else if(pdrv == FATFS_DEV_NUM_CTR_NAND)
	{
		if(dev_decnand->write(ctr_nand_offset + ((u32)sector<<9), (u32)count<<9, buff)) return RES_OK;
	}

	return RES_ERROR;
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
