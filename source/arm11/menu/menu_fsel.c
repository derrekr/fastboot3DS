/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200, d0k3
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
#include <ctype.h>
#include "types.h"
#include "fs.h"
#include "util.h"
#include "arm11/hardware/hid.h"
#include "fsutils.h"
#include "arm11/console.h"
#include "arm11/debug.h"
#include "arm11/fmt.h"

#define MAX_DIR_ENTRIES	0x200 // 512 -> worst case, approx 128kiB (yes, this is limited)
#define N_DIR_READ		0x10 // 16 at a time

typedef struct {
	u32 fsize;		// size of the file
	u8  is_dir;		// > 0 if is directory
	char* fname;	// filename (handle via malloc)
} DirBufferEntry;


static char* mallocpyString(const char* str)
{
	u32 strsize = strlen(str) + 1;
	char* astr = (char*) malloc(strsize);
	
	if (astr) strncpy(astr, str, strsize);
	return astr;
}


// inspired by http://www.geeksforgeeks.org/wildcard-character-matching/
static bool matchName(const char* path, const char* pattern)
{
    // handling non asterisk chars
    for (; *pattern != '*'; pattern++, path++)
	{
        if ((*pattern == '\0') && (*path == '\0'))
		{
            return true; // end reached simultaneously, match found
        }
		else if ((*pattern == '\0') || (*path == '\0'))
		{
            return false; // end reached on only one, failure
        }
		else if ((*pattern != '?') && (tolower(*pattern) != tolower(*path)))
		{
            return false; // chars don't match, failure
        }
    }
	
    // handling the asterisk (matches one or more chars in path)
    if ((*(pattern+1) == '?') || (*(pattern+1) == '*'))
	{
        return false; // stupid user/dev shenanigans, failure
    }
	else if (*path == '\0')
	{
        return false; // asterisk, but end reached on path, failure
    }
	else if (*(pattern+1) == '\0')
	{
        return true; // nothing after the asterisk, match found
    }
	else
	{
		// we couldn't really go without recursion here
        for (path++; *path != '\0'; path++)
		{
            if (matchName(path, pattern + 1) == true) return true;
        }
    }
    
    return false;
}


static void freeDirBufferContent(DirBufferEntry* dir_buffer, s32 n_entries)
{
	for(s32 i = 0; i < n_entries; i++)
	{
		if(dir_buffer[i].fname == NULL)
			break; // this should never happen
		
		free (dir_buffer[i].fname);
	}
}


static void sortDirBuffer(DirBufferEntry* dir_buffer, s32 n_entries)
{
	for(s32 s = 0; s < n_entries; s++)
	{
        DirBufferEntry* cmp0 = &(dir_buffer[s]);
        DirBufferEntry* min0 = cmp0;
		
        for(s32 c = s + 1; c < n_entries; c++)
		{
            DirBufferEntry* cmp1 = &(dir_buffer[c]);
            if(min0->is_dir != cmp1->is_dir)
			{
                if (min0->is_dir)
                    min0 = cmp1;
                continue;
            }
            if(strnicmp(min0->fname, cmp1->fname, 256) > 0)
			{
                min0 = cmp1;
			}
        }
		
        if(min0 != cmp0)
		{
            DirBufferEntry swap; // swap entries
            memcpy(&swap, cmp0, sizeof(DirBufferEntry));
            memcpy(cmp0, min0, sizeof(DirBufferEntry));
            memcpy(min0, &swap, sizeof(DirBufferEntry));
        }
    }
}


static s32 readDirToBuffer(DirBufferEntry* dir_buffer, const char* path, const char* pattern)
{
	FsFileInfo finfo[N_DIR_READ];
	s32 n_entries = 0;
	s32 dhandle;
	
	// set dir_buffer to all zeroes
	memset(dir_buffer, 0, sizeof(DirBufferEntry) * MAX_DIR_ENTRIES);

	// open directory
	dhandle = fOpenDir(path);
	if(dhandle < 0) return -1;
	
	s32 n_read = 0;
	while((n_read = fReadDir(dhandle, finfo, N_DIR_READ)) > 0)
	{
		if (n_read < 0) // error reading dir
			return -1;
		
		for(s32 i = 0; i < n_read; i++)
		{
			if(pattern && !matchName(finfo[i].fname, pattern))
				continue; // not a match with the provided pattern
			
			// take over data
			dir_buffer[n_entries].fsize = finfo[i].fsize;
			dir_buffer[n_entries].is_dir = finfo[i].fattrib & AM_DIR;
			dir_buffer[n_entries].fname = mallocpyString(finfo[i].fname);
			
			// check for out of memory
			if (!dir_buffer[n_entries].fname)
				return -1;
			
			// max dir buffer size reached?
			if (++n_entries >= MAX_DIR_ENTRIES)
				break;
		}
		
		// max dir buffer size reached?
		if (n_entries >= MAX_DIR_ENTRIES)
			break; // WARNING: no errors here - list just isn't complete
	}
	
	return n_entries;
}


u32 menuFileSelector(char* path, const char* start, const char* pattern)
{
	DirBufferEntry* dir_buffer;
	bool result = false;
	
	dir_buffer = (DirBufferEntry*) malloc(MAX_DIR_ENTRIES * sizeof(DirBufferEntry));
	if (!dir_buffer) return false;
	
	free(dir_buffer);
	
	return result;
}