/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/*-----------------------------------------------------------------------*/

#include <stdbool.h>
#include "diskio.h"		/* FatFs lower layer API */
#include "types.h"
#include "dev.h"

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
	u32 nand_offset;

	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			if(dev_sdcard->read((u32) sector << 9, (u32) count << 9, buff))
				return RES_OK;
			else
				return RES_ERROR;
		case FATFS_DEV_NUM_NAND:
			nand_offset = 0x0B95CA00;	// TODO
			if(dev_decnand->read(nand_offset + (u32) sector << 9, (u32) count << 9, buff))
				return RES_OK;
			else
				return RES_ERROR;
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
	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			if(dev_sdcard->write((u32) sector << 9, (u32) count << 9, buff))
				return RES_OK;
			else
				return RES_ERROR;
		case FATFS_DEV_NUM_NAND:
			if(dev_decnand->write((u32) sector << 9, (u32) count << 9, buff))
				return RES_OK;
			else
				return RES_ERROR;
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
