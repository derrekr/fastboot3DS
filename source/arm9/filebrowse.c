#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/fatfs/ff.h"
#include "arm9/console.h"
#include "arm9/main.h"
#include "arm9/interrupt.h"
#include "util.h"
#include "hid.h"

#define MAX_CACHED_ENTRIES	0x400
#define MAX_ENTRY_SIZE		_MAX_LFN + 2
#define MAX_NEW_ENTRIES		0x20
#define MAX_PATH_LENGTH		0x400
#define	MAX_PATH_DEPTH		0x20

typedef struct {
	char name[MAX_ENTRY_SIZE];
	bool isDir;
} EntryType;

static EntryType *dirEntries; // [MAX_ENTRY_SIZE][MAX_CACHED_ENTRIES];
static DIR	curDirectory;
static char	curPath[MAX_PATH_LENGTH];
static u32	curEntriesCount;

// This stack is used to recover the original cursor position
// in the dir entries list when returning from a subdir.
static u32	indexStack[MAX_PATH_DEPTH];
static u32 *indexStackPtr = indexStack;

/*	This scans the current directory for entries.
	Only MAX_NEW_ENTRIES will be added at maximum.
	This also updates the curEntriesCount. 
	Returns true if new entries were added. */
static bool scanDirectory()
{
	FRESULT res;
	static FILINFO fno;
	unsigned i = 0;
	
	for(i=0; i<MAX_NEW_ENTRIES; i++)
	{
		if(curEntriesCount + i == MAX_CACHED_ENTRIES) break;
		
		res = f_readdir(&curDirectory, &fno);
		
		if(res != FR_OK || fno.fname[0] == 0) break;
		
		dirEntries[curEntriesCount + i].isDir = (fno.fattrib & AM_DIR) != 0;
		
		if(fno.fattrib & AM_DIR)	// directory
			snprintf(dirEntries[curEntriesCount + i].name, _MAX_LFN + 1, "\\%s", fno.fname);
		else	// file
			strncpy(dirEntries[curEntriesCount + i].name, fno.fname, _MAX_LFN + 1);
	}
	
	curEntriesCount += i;
	
	return i != 0;
}

static u32 ascendPath()
{
	char *end = curPath + strlen(curPath);
	
	for(; end != curPath + 1; end--)
		if(*end == '\\' || *(end - 1) == ':') break;
		
	*end = '\0';
	
	if(indexStackPtr != indexStack) indexStackPtr--;
	return *indexStackPtr;
}

static bool descendPath(u32 savedIndex, char *subdir)
{
	if(indexStackPtr < indexStack + MAX_PATH_DEPTH)
		*indexStackPtr++ = savedIndex;
	else return false;	// out of stack
	
	strncat(curPath, subdir, sizeof(curPath) - strlen(curPath) + 1);
	
	return true;
}

static void formatEntry(char *out, const char* in, const u32 bufSize, bool shortenEnd)
{
	const char *end;
	const u32 entryLen = strlen(in);
	
	if(!shortenEnd)
	{
		if(entryLen > bufSize - 1)
		{
			end = in + entryLen;
			strcpy(&out[3], end - bufSize + 3);
			memcpy(out, "...", 3);
		}
		else strcpy(out, in);
	}
	else
	{
		if(entryLen > bufSize - 1)
		{
			memcpy(out, in, bufSize - 4);
			memcpy(&out[bufSize - 5], "...", 4);
		}
		else strcpy(out, in);
	}
}

static void updateScreen(u32 cursor_pos)
{
	// we don't want ugly line breaks in our list
	const int maxLen = con_bottom.windowWidth - 1;
	const int maxEntries = con_bottom.windowHeight - 1;
	
	char entry [maxLen];	// should be safe
	unsigned start, end;
	
	clearConsoles();
	consoleSelect(&con_bottom);
	
	// print the current path in the first line
	formatEntry(entry, curPath, (u32) maxLen - 3, false);
	printf("%s :", entry);
	
	if(cursor_pos < (u32) maxEntries)
	{
		start = 0;
		end = min(maxEntries, curEntriesCount);
	}
	else
	{
		start = cursor_pos - (u32) maxEntries + 1;
		end = min(cursor_pos + 1, curEntriesCount);
	}
	
	for(unsigned i = start; i < end; i++)
	{
		formatEntry(entry, dirEntries[i].name, (u32) maxLen - 2, true);
		printf("\n%s %s", i == cursor_pos ? "*" : " ", entry);
	}
}

const char *browseForFile(const char *basePath)
{
	FRESULT res;
	u32 keys;
	u32 cursor_pos = 0;
	
	curEntriesCount = 0;
	indexStackPtr = indexStack;
	
	dirEntries = (EntryType *) malloc(MAX_CACHED_ENTRIES);
	if(!dirEntries) return NULL;
	
	if(basePath)
		strncpy(curPath, basePath, sizeof(curPath));
	else
		strncpy(curPath, "sdmc:", sizeof(curPath));
	
	res = f_opendir(&curDirectory, curPath);
	if (res != FR_OK)
	{
		free(dirEntries);
		return NULL;
	}
	
	// do an initial scan
	scanDirectory();
	
	updateScreen(cursor_pos);
	
	for(;;)
	{
		hidScanInput();
		keys = hidKeysDown()/* | hidKeysHeld()*/;
		
		if(curEntriesCount != 0)
		{
			if(keys & KEY_DUP)
			{
				if(cursor_pos != 0) cursor_pos--;
				updateScreen(cursor_pos);
				sleep_wait(0x1000);
			}
			else if(keys & KEY_DDOWN)
			{
				// we reached the end of our entries list?
				if(cursor_pos == curEntriesCount - 1)
				{
					scanDirectory();
					// we got more entries?
					if(cursor_pos < curEntriesCount - 1)
						cursor_pos++;
				}
				else cursor_pos++;
				updateScreen(cursor_pos);
				sleep_wait(0x1000);
			}
			
			else if(keys & KEY_A)	// select entry
			{
				if(dirEntries[cursor_pos].isDir)	// it's a dir
				{
					if(descendPath(cursor_pos, dirEntries[cursor_pos].name))
					{
						f_closedir(&curDirectory);
						res = f_opendir(&curDirectory, curPath);
						if (res != FR_OK)
							ascendPath();
						else
						{
							// fetch new entries
							curEntriesCount = 0;
							scanDirectory();
							cursor_pos = 0;
							updateScreen(cursor_pos);
						}
					}
				}
				else	// we got our file!
				{
					if(strlen(dirEntries[cursor_pos].name) + strlen(curPath) + 2 > sizeof(curPath))
						goto fail;	// TODO? Path too long.
					strcat(curPath, "\\");
					strcat(curPath, dirEntries[cursor_pos].name);
					break;
				}
			}
		}
		if(keys & KEY_B)	// go back
		{
			if(indexStackPtr != indexStack)
			{
				cursor_pos = ascendPath();
				f_closedir(&curDirectory);
				
				res = f_opendir(&curDirectory, curPath);
				if (res != FR_OK)
					goto fail;
				curEntriesCount = 0;
				
				do {
					if(!scanDirectory())
						goto fail;
				} while(curEntriesCount <= cursor_pos);
				
				updateScreen(cursor_pos);
			}
			else goto fail;
		}

		__asm__("mcr p15, 0, %[in], c7, c0, 4\n" : : [in] "r" (0));
		REG_IRQ_IF = (u32)INTERRUPT_TIMER_0;
	}
	
	f_closedir(&curDirectory);
	free(dirEntries);
	
	return curPath;
	
fail:
	f_closedir(&curDirectory);
	free(dirEntries);
	return NULL;
}
