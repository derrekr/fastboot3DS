#pragma once

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


#define MAX_PARTITIONS				8
#define MAX_PARTITION_NAME			10
#define PARTITION_INVALID_INDEX		-1

typedef struct {
	char name[MAX_PARTITION_NAME + 1];
	u32 sector;
	u32 count;
	u8  type;
	u8  keyslot;
} partitionStruct;



#ifdef __cplusplus
extern "C"
{
#endif

size_t partitionAdd(u32 sector, u32 count, u8 type);
bool partitionSetName(size_t index, const char *name);
bool partitionFind(u32 sector, u32 count, size_t *index);
bool partitionGetIndex(const char *name, size_t *index);
bool partitionGetSectorOffset(size_t index, size_t *offset);
bool partitionSetKeyslot(size_t index, u8 keyslot);
bool partitionGetKeyslot(size_t index, u8 *keyslot);
bool partitionGetInfo(size_t index, partitionStruct *info);
void partitionsReset(void);

#ifdef __cplusplus
}
#endif
