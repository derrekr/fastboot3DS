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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fatfs/ff.h"
#include "arm9/debug.h"
#include "arm9/dev.h"
#include "arm9/partitions.h"
#include "util.h"

/* Keeps track of all NAND partitions */

static partitionStruct partitions[MAX_PARTITIONS];
static size_t numPartitions;



static inline int findPartition(const char *name)
{
	size_t namelen = strlen(name);

	if(namelen >= 1 && name[namelen - 1] == ':') namelen--;

	for(size_t i=0; i<numPartitions; i++)
	{
		// ee_printf("findPartition: %s vs %s\n", name, partitions[i].name);
		if(strlen(partitions[i].name) == namelen &&
			memcmp(name, partitions[i].name, namelen) == 0)
			return i;
	}
	
	return -1;
}

// Adds a partition and returns index.
size_t partitionAdd(u32 sector, u32 count, u8 type)
{
	size_t index;
	partitionStruct *part;
	
	if(numPartitions >= MAX_PARTITIONS)
		panic();

	index = numPartitions;

	part = &partitions[index];
	part->sector = sector;
	part->count = count;
	part->type = type;
	
	numPartitions++;

	return index;
}

bool partitionSetName(size_t index, const char *name)
{
	size_t otherIndex;
	partitionStruct *part;
	char tempName[MAX_PARTITION_NAME + 1];
	
	if(index >= MAX_PARTITIONS)
		return false;
	
	strncpy(tempName, name, MAX_PARTITION_NAME);
	tempName[MAX_PARTITION_NAME] = '\0';
	
	if((otherIndex = findPartition(tempName)) != (size_t) -1)
		return false;
	
	part = &partitions[index];
	strcpy(part->name, tempName);
	
	return true;
}

bool partitionFind(u32 sector, u32 count, size_t *index)
{
	partitionStruct *part;
	
	for(size_t i = 0; i < MAX_PARTITIONS; i++)
	{
		part = &partitions[i];
		if((part->sector <= sector) && (part->count >= count)
			&& (part->sector + part->count >= sector + count))
		{
			*index = i;
			return true;
		}
	}

	*index = PARTITION_INVALID_INDEX;
	return false;
}

// Converts name into index
bool partitionGetIndex(const char *name, size_t *index)
{
	size_t i;
	
	if((i = findPartition(name)) != (size_t) -1)
	{
		*index = i;
		return true;
	}

	*index = PARTITION_INVALID_INDEX;
	return false;
}

bool partitionGetSectorOffset(size_t index, size_t *offset)
{
	if(index >= MAX_PARTITIONS)
		return false;
	
	*offset = partitions[index].sector;
	return true;
}

bool partitionSetKeyslot(size_t index, u8 keyslot)
{
	if(index >= MAX_PARTITIONS)
		return false;
	
	partitions[index].keyslot = keyslot;
	return true;
}

bool partitionGetKeyslot(size_t index, u8 *keyslot)
{
	if(index >= MAX_PARTITIONS)
	{
		*keyslot = 0xFF;
		return false;
	}
	
	*keyslot = partitions[index].keyslot;
	return true;
}

bool partitionGetInfo(size_t index, partitionStruct *info)
{
	if(index >= MAX_PARTITIONS)
	{
		memset(info, 0, sizeof(*info));
		return false;
	}
	
	memcpy(info, &partitions[index], sizeof(*info));
	
	return true;
}

void partitionsReset(void)
{
	numPartitions = 0;

	for(u32 i = 0; i < MAX_PARTITIONS; i++)
	{
		partitions[i].name[0] = '\0';
		partitions[i].sector = 0;
		partitions[i].count = 0;
		partitions[i].type = 0;
		partitions[i].keyslot = 0xFF;
	}
}
