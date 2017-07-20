/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"		/* FatFs lower layer API */
#include "types.h"
#include "arm9/dev.h"

// Get's set externally in dev.c
u32 ctr_nand_sector;

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
	DSTATUS stat = 0;

	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			if(!dev_sdcard->is_active() || !dev_sdcard->initialized) stat = STA_NOINIT;
			break;
		case FATFS_DEV_NUM_TWL_NAND:
			if(!dev_decnand->is_active()) stat = STA_NOINIT;
			break;
		case FATFS_DEV_NUM_CTR_NAND:
			if(!dev_decnand->is_active()) stat = STA_NOINIT;
			break;
		default:
			stat = STA_NOINIT;
	}

	return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	DSTATUS stat = 0;

	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			if(!dev_sdcard->init()) stat = STA_NOINIT;
			break;
		case FATFS_DEV_NUM_TWL_NAND:
			if(!dev_decnand->init()) stat = STA_NOINIT;
			break;
		case FATFS_DEV_NUM_CTR_NAND:
			if(!dev_decnand->init()) stat = STA_NOINIT;
			break;
		default:
			stat = STA_NOINIT;
	}

	return stat;
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
	DRESULT res = RES_OK;

	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			if(!dev_sdcard->read_sector((u32)sector, (u32)count, buff)) res = RES_ERROR;
			break;
		case FATFS_DEV_NUM_TWL_NAND:
			if(!dev_decnand->read_sector((u32)sector, (u32)count, buff)) res = RES_ERROR;
			break;
		case FATFS_DEV_NUM_CTR_NAND:
			if(!dev_decnand->read_sector(ctr_nand_sector + (u32)sector, (u32)count, buff)) res = RES_ERROR;
			break;
		default:
			res = RES_PARERR;
	}

	return res;
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
	DRESULT res = RES_OK;

	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			if(!dev_sdcard->write_sector((u32)sector, (u32)count, buff)) res = RES_ERROR;
			break;
		case FATFS_DEV_NUM_TWL_NAND:
			if(!dev_decnand->write_sector((u32)sector, (u32)count, buff)) res = RES_ERROR;
			break;
		case FATFS_DEV_NUM_CTR_NAND:
			if(!dev_decnand->write_sector(ctr_nand_sector + (u32)sector, (u32)count, buff)) res = RES_ERROR;
			break;
		default:
			res = RES_PARERR;
	}

	return res;
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
	const dev_struct *dev;

	switch(pdrv)
	{
		case FATFS_DEV_NUM_SD:
			dev = dev_sdcard;
			break;
		case FATFS_DEV_NUM_TWL_NAND:
			dev = dev_decnand;
			break;
		case FATFS_DEV_NUM_CTR_NAND:
			dev = dev_decnand;
			break;
		default:
			return RES_PARERR;
	}

	DRESULT res = RES_OK;
	switch(cmd)
	{
		case GET_SECTOR_COUNT:
			if(dev->get_sector_count)
			{
				*(DWORD*)buff = dev->get_sector_count();
				break;
			}
			res = RES_NOTRDY;
			break;
		case GET_SECTOR_SIZE:
			*(WORD*)buff = 512;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 0x100; // Default to 128 KB
		case CTRL_TRIM:
		case CTRL_SYNC:
			break;
		default:
			res = RES_PARERR;
	}

	return res;
}

