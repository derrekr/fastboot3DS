/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <malloc.h>
#include "types.h"
#include "util.h"
#include "fs.h"
#include "arm9/debug.h"
#include "arm9/dev.h"
#include "arm9/ncsd.h"
#include "arm9/partitions.h"
#include "fatfs/ff.h"


typedef struct
{
	u8 *mem;
	size_t memSize;
	size_t dataSize;
} DevBuf;

typedef struct
{
	size_t sector;
	size_t count;
} ProtNandRegion;


static const DevHandle devHandleMagic = 0x42424296;

static FATFS fsTable[FS_MAX_DRIVES] = {0};
static const char *const fsPathTable[FS_MAX_DRIVES] = {FS_DRIVE_NAMES};
static bool fsStatTable[FS_MAX_DRIVES] = {0};

static FIL fTable[FS_MAX_FILES] = {0};
static bool fStatTable[FS_MAX_FILES] = {0};
static u32 fHandles = 0;

static DIR dTable[FS_MAX_DIRS] = {0};
static bool dStatTable[FS_MAX_DIRS] = {0};
static u32 dHandles = 0;

static bool devStatTable[FS_MAX_DEVICES] = {0};
static bool fsStatBackupTable[FS_MAX_DRIVES] = {0};

static DevBuf devBuf;

static ProtNandRegion protNandRegions[MAX_PARTITIONS + 2]  = {0};
static size_t numProtNandRegions;



static bool isFileHandleValid(s32 handle);

static inline bool isNandProtected()
{
	return numProtNandRegions != 0;
}

static inline ProtNandRegion *getNandProtRegion(size_t startSector, size_t writeSize)
{
	ProtNandRegion *region;
	size_t regionEnd, writeEnd;
	
	if(startSector > ~writeSize)
		return NULL;
	
	if(writeSize <= 0)
		return NULL;
	
	writeEnd = startSector + writeSize;

	for(size_t i=0; i < numProtNandRegions; i++)
	{
		region = &protNandRegions[i];
		regionEnd = region->sector + region->count;
		
		if(startSector < regionEnd && writeEnd > region->sector)
			return region;
	}
	
	return NULL;
}

s32 fMount(FsDrive drive)
{
	if((u32)drive >= FS_MAX_DRIVES) return -30;
	if(fsStatTable[drive]) return -31;

	FRESULT res = f_mount(&fsTable[drive], fsPathTable[drive], 1);
	if(res == FR_OK)
	{
		fsStatTable[drive] = true;
		return FR_OK;
	}
	else return -res;
}

s32 fUnmount(FsDrive drive)
{
	if((u32)drive >= FS_MAX_DRIVES) return -30;
	if(!fsStatTable[drive]) return -31;

	FRESULT res = f_mount(NULL, fsPathTable[drive], 0);
	fsStatTable[drive] = false;

	if(res == FR_OK) return FR_OK;
	else return -res;
}

bool fIsDriveMounted(FsDrive drive)
{
	if((u32)drive >= FS_MAX_DRIVES) return false;
	
	return fsStatTable[drive];
}

static s32 ensureUnmounted(FsDrive drive)
{
	if(!fIsDriveMounted(drive))
		return FR_OK;
	
	return fUnmount(drive);
}

static s32 ensureMounted(FsDrive drive)
{
	if(fIsDriveMounted(drive))
		return FR_OK;
	
	return fMount(drive);
}

s32 fGetFree(FsDrive drive, u64 *size)
{
	if((u32)drive >= FS_MAX_DRIVES) return -30;
	if(!fsStatTable[drive]) return -31;

	DWORD freeClusters;
	FATFS *fs;
	FRESULT res = f_getfree(fsPathTable[drive], &freeClusters, &fs);
	if(res == FR_OK)
	{
		if(size) *size = ((u64)(freeClusters * fs->csize)) * 512;
		return FR_OK;
	}
	else return -res;
}

u32 fGetDeviceSize(FsDevice dev)
{
	switch(dev)
	{
		case FS_DEVICE_SDMC:
			return dev_sdcard->get_sector_count();
			break;
		case FS_DEVICE_NAND:
			return dev_rawnand->get_sector_count();
			break;
		default:
			break;
	}
	
	return 0;
}

bool fIsDevActive(FsDevice dev)
{
	switch(dev)
	{
		case FS_DEVICE_SDMC:
			return dev_sdcard->is_active();
			break;
		case FS_DEVICE_NAND:
			return dev_rawnand->is_active();
			break;
		default:
			break;
	}
	
	return false;
}

s32 fPrepareRawAccess(FsDevice dev)
{
	s32 err;

	if((u32)dev >= FS_MAX_DEVICES) return -30;
	if(devStatTable[dev]) return -31;
	
	if(dev != FS_DEVICE_NAND)
		return -31;
	
	memcpy(fsStatBackupTable, fsStatTable, sizeof(fsStatTable));
	
	switch(dev)
	{
		case FS_DEVICE_SDMC:
			err = ensureUnmounted(FS_DRIVE_SDMC);
			if(err != FR_OK) return err;
			break;
		case FS_DEVICE_NAND:
			err = ensureUnmounted(FS_DRIVE_TWLN);
			if(err != FR_OK) return err;
			err = ensureUnmounted(FS_DRIVE_TWLP);
			if(err != FR_OK) return err;
			err = ensureUnmounted(FS_DRIVE_NAND);
			if(err != FR_OK) return err;
			break;
		default:
			return -30; //panic();
	}
	
	devStatTable[dev] = true;
	
	return devHandleMagic;
}

static inline bool isValidDevHandle(DevHandle handle)
{
	if(handle != devHandleMagic)
		return false;
	
	return true;
}

static inline FsDevice getDeviceFromHandle(DevBufHandle handle)
{
	// for now only nand is supported.
	(void) handle;
	return FS_DEVICE_NAND;
}

static inline bool usesRawAccess(FsDevice dev)
{
	if(!devStatTable[dev])
		return false;
	
	return true;
}

s32 fFinalizeRawAccess(DevHandle handle)
{
	FsDevice dev;
	s32 err = FR_OK;
	
	if(!isValidDevHandle(handle)) return -30;
	
	dev = getDeviceFromHandle(handle);	
	
	if(!usesRawAccess(dev)) return -31;
	
	for(u32 drive = 0; drive < FS_MAX_DRIVES; drive++)
	{
		if(fsStatBackupTable[drive])
		{
			err = ensureMounted(drive);
			if(err != FR_OK) goto fail;
		}
	}
	
	devStatTable[dev] = false;
	
fail:
	
	return err;
}

static bool devBufAllocate(DevBuf *devBuf, u32 size)
{
	devBuf->mem = malloc(size);
	if(!devBuf->mem) return false;
	
	devBuf->memSize = size;
	
	return true;
}

s32 fCreateDeviceBuffer(u32 size)
{
	if(!size || size > 0x180000) return -30;
	if(devBuf.mem) return -31;
	
	if(!devBufAllocate(&devBuf, size))
		 return -30;

	return FR_OK;
}

static bool isValidDevBufHandle(DevBufHandle handle)
{
	if(handle != FR_OK)
		return false;
	
	if(!devBuf.mem)
		return false;
	
	return true;
}

static bool devBufFree(DevBuf *devBuf)
{
	free(devBuf->mem);
	devBuf->mem = NULL;
	
	devBuf->memSize = 0;
	
	return true;
}

s32 fFreeDeviceBuffer(DevBufHandle handle)
{
	if(!isValidDevBufHandle(handle)) return -30;
	
	devBufFree(&devBuf);
	
	return FR_OK;
}

// Reads from a device or file to a device buffer
// Note: size must be <= cache size, else: error
s32 fReadToDeviceBuffer(s32 sourceHandle, u32 sourceOffset, u32 sourceSize, DevBufHandle devBufHandle)
{
	FsDevice dev;
	u32 sector, count;
	bool fromFile;
	
	// source is a device?
	if(isValidDevHandle(sourceHandle))
	{
		if(!isValidDevHandle(sourceHandle))
			return -30;
	
		dev = getDeviceFromHandle(sourceHandle);
	
		if(!usesRawAccess(dev))
			return -30;
		
		if(sourceOffset % 0x200 || sourceSize % 0x200)
			return -30;
		
		fromFile = false;
	}
	else
	{
		// source must be file, but is it valid?
		if(!isFileHandleValid(sourceHandle))
			return -30;
		
		fromFile = true;
	}
	
	/* validate device buffer */
	
	if(!isValidDevBufHandle(devBufHandle))
		return -30;
	
	if(devBuf.memSize < sourceSize)
		return -30;
	
	/* getting interesting here */
	
	if(fromFile)
	{
		if(fLseek(sourceHandle, sourceOffset) < 0)
			return -31;
		
		if(fRead(sourceHandle, devBuf.mem, sourceSize) < 0)
			return -31;
	}
	else
	{
		// for now...
		if(dev != FS_DEVICE_NAND)
			return -30;
		
		if(!dev_rawnand->is_active())
			return -31;
		
		sector = sourceOffset >> 9;
		count = sourceSize >> 9;
		
		if(!dev_rawnand->read_sector(sector, count, devBuf.mem))
			return -31;
	}
	
	devBuf.dataSize = sourceSize;
	
	return FR_OK;
}

// Writes from a device buffer to a device or file.
// Note: size must be <= cache size, else: error
s32 fsWriteFromDeviceBuffer(s32 destHandle, u32 destOffset, u32 destSize, DevBufHandle devBufHandle)
{
	FsDevice dev;
	u32 sector, count;
	bool toFile;
	const ProtNandRegion *region;
	
	// destination is a device?
	if(isValidDevHandle(destHandle))
	{
		if(!isValidDevHandle(destHandle))
			return -30;
	
		dev = getDeviceFromHandle(destHandle);
	
		if(!usesRawAccess(dev))
			return -30;
		
		if(destOffset % 0x200 || destSize % 0x200)
			return -30;
		
		toFile = false;
	}
	else
	{
		// dest must be file, but is it valid?
		if(!isFileHandleValid(destHandle))
			return -30;
		
		toFile = true;
	}
	
	/* validate device buffer */
	
	if(!isValidDevBufHandle(devBufHandle))
		return -30;
	
	if(devBuf.dataSize < destSize)
		return -30;
	
	count = min(devBuf.dataSize, destSize);
	
	if(toFile)
	{
		if(fLseek(destHandle, destOffset) < 0)
			return -31;
		
		if(fWrite(destHandle, devBuf.mem, count) < 0)
			return -31;
	}
	else
	{
		// for now...
		if(dev != FS_DEVICE_NAND)
			return -30;
		
		if(!dev_rawnand->is_active())
			return -31;
		
		if(count % 0x200)
			return -30;
		
		sector = destOffset >> 9;
		count = count >> 9;
		
		
		if(isNandProtected())
		{
			u32 toWrite = count;
			u8 *devBufPtr = devBuf.mem;
			
			/* check if we want to write to a protected area on NAND */
			
			do
			{
				region = getNandProtRegion(sector, toWrite);
				
				if(region)
				{
					// we're inside a prot region?
					if(region->sector <= sector)
					{
						// calc how much do we need to skip
						count = min(region->sector + region->count, sector + toWrite) - sector;
					}
					else	// we are going to run into a prot region
					{
						count = min(toWrite, region->sector - sector);
						
						if(!dev_rawnand->write_sector(sector, count, devBufPtr))
							return -31;
					}
				}
				else
				{
					count = toWrite;
					
					// no prot regions found, do a normal write
					if(!dev_rawnand->write_sector(sector, count, devBufPtr))
						return -31;
				}
				
				devBufPtr += count << 9;
				sector += count;
				toWrite -= count;
			}
			while(toWrite);
		}
		else
		{
			if(!dev_rawnand->write_sector(sector, count, devBuf.mem))
				return -31;
		}
	}

	devBuf.dataSize = 0;
	
	return FR_OK;
}

static s32 findUnusedFileSlot(void)
{
	if(fHandles >= FS_MAX_FILES) return -1;

	s32 i = 0;
	while(i < FS_MAX_FILES)
	{
		if(!fStatTable[i]) break;
		i++;
	}

	if(i == FS_MAX_FILES) return -1;
	else return i;
}

static bool isFileHandleValid(s32 handle)
{
	if((u32)handle > fHandles) return false;
	else return true;
}

s32 fOpen(const char *const path, FsOpenMode mode)
{
	const s32 i = findUnusedFileSlot();
	if(i < 0) return -30;

	FRESULT res = f_open(&fTable[i], path, mode);
	if(res == FR_OK)
	{
		fStatTable[i] = true;
		fHandles++;
		return i; // Handle
	}
	else return -res;
}

s32 fRead(s32 handle, void *const buf, u32 size)
{
	if(!isFileHandleValid(handle)) return -30;

	UINT bytesRead;
	FRESULT res = f_read(&fTable[handle], buf, size, &bytesRead);

	if(bytesRead != size) return -31;
	if(res == FR_OK) return FR_OK;
	else return -res;
}

s32 fWrite(s32 handle, const void *const buf, u32 size)
{
	if(!isFileHandleValid(handle)) return -30;

	UINT bytesWritten;
	FRESULT res = f_write(&fTable[handle], buf, size, &bytesWritten);

	if(bytesWritten != size) return -31;
	if(res == FR_OK) return FR_OK;
	else return -res;
}

s32 fSync(s32 handle)
{
	if(!isFileHandleValid(handle)) return -30;

	FRESULT res = f_sync(&fTable[handle]);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fLseek(s32 handle, u32 offset)
{
	if(!isFileHandleValid(handle)) return -30;

	FRESULT res = f_lseek(&fTable[handle], offset);
	if(res == FR_OK) return res;
	else return -res;
}

u32 fTell(s32 handle)
{
	if(!isFileHandleValid(handle)) return 0;
	return f_tell(&fTable[handle]);
}

u32 fSize(s32 handle)
{
	if(!isFileHandleValid(handle)) return 0;
	return f_size(&fTable[handle]);
}

s32 fClose(s32 handle)
{
	if(fHandles == 0 || !isFileHandleValid(handle)) return -30;

	FRESULT res = f_close(&fTable[handle]);
	fStatTable[handle] = false;
	fHandles--;

	if(res == FR_OK) return FR_OK;
	else return -res;
}

s32 fExpand(s32 handle, u32 size)
{
	if(!isFileHandleValid(handle)) return -30;

	FRESULT res = f_expand(&fTable[handle], size, 1);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fStat(const char *const path, FsFileInfo *fi)
{
	FRESULT res = f_stat(path, fi);
	if(res == FR_OK) return res;
	else return -res;
}

static s32 findUnusedDirSlot(void)
{
	if(dHandles >= FS_MAX_DIRS) return -1;

	s32 i = 0;
	while(i < FS_MAX_DIRS)
	{
		if(!dStatTable[i]) break;
		i++;
	}

	if(i == FS_MAX_DIRS) return -1;
	else return i;
}

static bool isDirHandleValid(s32 handle)
{
	if((u32)handle > dHandles) return false;
	else return true;
}

s32 fOpenDir(const char *const path)
{
	const s32 i = findUnusedDirSlot();
	if(i < 0) return -30;

	FRESULT res = f_opendir(&dTable[i], path);
	if(res == FR_OK)
	{
		dStatTable[i] = true;
		dHandles++;
		return i; // Handle
	}
	else return -res;
}

s32 fReadDir(s32 handle, FsFileInfo *fi, u32 num)
{
	if(!isDirHandleValid(handle)) return -30;
	if(num > 1000) return -31;

	u32 i;
	for(i = 0; i < num; i++)
	{
		FRESULT res = f_readdir(&dTable[handle], &fi[i]);
		if(res != FR_OK) return -res;
		if(!fi[i].fname[0]) break;
	}

	return i;
}

s32 fCloseDir(s32 handle)
{
	if(dHandles == 0 || !isDirHandleValid(handle)) return -30;

	FRESULT res = f_closedir(&dTable[handle]);
	dStatTable[handle] = false;
	dHandles--;

	if(res == FR_OK) return FR_OK;
	else return -res;
}

s32 fMkdir(const char *const path)
{
	FRESULT res = f_mkdir(path);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fRename(const char *const old, const char *const new)
{
	FRESULT res = f_rename(old, new);
	if(res == FR_OK) return res;
	else return -res;
}

s32 fUnlink(const char *const path)
{
	FRESULT res = f_unlink(path);
	if(res == FR_OK) return res;
	else return -res;
}

static size_t calcNandImageMinSize(const NCSD_header *header)
{
	const u32 mediaSize = header->mediaSize;
	const struct NCSD_part *part = header->partitions;
	u32 partSize, partOffset, total;
	size_t largest = 0;
	
	for(size_t i=0; i<arrayEntries(header->partitions); i++, part++)
	{
		partSize = part->mediaSize;
		partOffset = part->mediaOffset;
		
		if(partOffset > ~partSize)
			goto fail;
		
		total = partOffset + partSize;
		
		if(largest < total)
			largest = total;
	}
	
	if(UINT32_MAX / 0x200 < largest)
		goto fail;
	
	if(largest > mediaSize)
		goto fail;
	
	return largest * 0x200;
	
fail:

	return 0;
}

s32 fVerifyNandImage(const char *const path)
{
	const u32 maxImageSize = fGetDeviceSize(FS_DEVICE_NAND) << 9;
	u32 minImageSize = 0x200;

	NCSD_header imageHeader;
	NCSD_header physicalHeader;
	s32 fHandle;
	u32 imageSize;
	s32 ret = -30;

	fHandle = fOpen(path, FS_OPEN_READ);
	if(fHandle < 0) return ret;
	
	imageSize = fSize(fHandle);
	
	if(imageSize < minImageSize || imageSize > maxImageSize)
		goto done;	
	
	if(fRead(fHandle, &imageHeader, sizeof(NCSD_header)) != FR_OK)
		goto done;
	
	if(!dev_rawnand->read_sector(0, 1, &physicalHeader))
		goto done;
	
	/* compare everything except the signature */
	if(memcmp(&imageHeader.magic, &physicalHeader.magic,
				sizeof(imageHeader) - sizeof(imageHeader.signature)))
		goto done;
	
	minImageSize = calcNandImageMinSize(&imageHeader);
	
	if(!minImageSize || imageSize < minImageSize)
		goto done;
	
	/* success! */
	
	ret = FR_OK;
	
done:

	fClose(fHandle);
	
	return ret;
}

s32 fSetNandProtection(bool protect)
{
	static const ProtNandRegion defaultProt[] = {
		{
			.sector = 0,
			.count = 1,
		},
		{
			.sector = 0x96,
			.count = 1,
		}
	};

	ProtNandRegion *region;
	partitionStruct partInfo;

	if(protect == isNandProtected())	// nothing to do here
		return FR_OK;
	
	if(protect)
	{
		numProtNandRegions = arrayEntries(defaultProt);
		memcpy(protNandRegions, defaultProt, sizeof defaultProt);
		
		const size_t maxPartitions = arrayEntries(protNandRegions) - arrayEntries(defaultProt);
	
		for(size_t i=0; i < maxPartitions; i++)
		{
			if(partitionGetInfo(i, &partInfo))
			{
				if(partInfo.type != 3) // is not a firmware?
					continue;
				
				/* Check if we can merge two adjacent regions */
				if(partInfo.sector)
					region = getNandProtRegion(partInfo.sector - 1, 1);
				else region = NULL;
				
				/* Found a previous region, merge */
				if(region)
				{
					region->count += partInfo.count;
				}
				else	/* default case, register a new region */
				{
					protNandRegions[numProtNandRegions].sector = partInfo.sector;
					protNandRegions[numProtNandRegions].count = partInfo.count;
					numProtNandRegions++;
				}
			}
		}
	}
	else
	{
		numProtNandRegions = 0;
	}

	return FR_OK;
}

void fsDeinit(void)
{
	for(u32 i = 0; i < FS_MAX_FILES; i++) fClose(i);
	for(u32 i = 0; i < FS_MAX_DRIVES; i++) fUnmount(i);

	dev_decnand->close();
	dev_rawnand->close();
	dev_sdcard->close();
}
