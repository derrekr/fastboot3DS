#pragma once

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

size_t partitionAdd(u32 sector, u32 count, u8 type);
bool partitionSetName(size_t index, const char *name);
bool partitionFind(u32 sector, u32 count, size_t *index);
bool partitionGetIndex(const char *name, size_t *index);
bool partitionGetSectorOffset(size_t index, size_t *offset);
bool partitionSetKeyslot(size_t index, u8 keyslot);
bool partitionGetKeyslot(size_t index, u8 *keyslot);
