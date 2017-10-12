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
#include "fsutils.h"
#include "arm11/menu/menu_fsel.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/timer.h"
#include "arm11/console.h"
#include "arm11/power.h"
#include "arm11/debug.h"
#include "arm11/fmt.h"

#define MAX_DIR_ENTRIES	0x100 // 256 -> worst case approx 64kiB (yes, this is limited)
#define N_DIR_READ		0x10  // 16 at a time

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


void truncateString(char* dest, const char* orig, int nsize, int tpos)
{
    int osize = strlen(orig);
	
    if (nsize < 0)
	{
        return;
    } else if (nsize <= 3)
	{
        ee_snprintf(dest, nsize, orig);
    } else if (nsize >= osize)
	{
        ee_snprintf(dest, nsize + 1, orig);
    } else
	{
        if (tpos + 3 > nsize) tpos = nsize - 3;
        ee_snprintf(dest, nsize + 1, "%-.*s...%-.*s", tpos, orig, nsize - (3 + tpos), orig + osize - (nsize - (3 + tpos)));
    }
}


void formatBytes(char* str, u64 bytes)
{
	// str should be 32 byte in size, just to be safe
    const char* units[] = {"  Byte", " kiB", " MiB", " GiB"};
    
    if (bytes < 1024)
	{
		ee_snprintf(str, 32, "%llu%s", bytes, units[0]);
	}
    else
	{
        u32 scale = 1;
        u64 bytes100 = (bytes * 100) >> 10;
        for(; (bytes100 >= 1024*100) && (scale < 3); scale++, bytes100 >>= 10);
        ee_snprintf(str, 32, "%llu.%llu%s", bytes100 / 100, (bytes100 % 100) / 10, units[scale]);
    }
}


// inspired by http://www.geeksforgeeks.org/wildcard-character-matching/
static bool matchName(const char* path, const char* pattern)
{
	// handling non asterisk chars
	for (; *pattern != '*'; pattern++, path++)
	{
		if ((*pattern == '\0') && (*path == '\0'))
		{
			// end reached simultaneously, match found
			return true;
		}
		else if ((*pattern == '\0') || (*path == '\0'))
		{
			// end reached on only one, failure
			return false;
		}
		else if ((*pattern != '?') && (tolower(*pattern) != tolower(*path)))
		{
			// chars don't match, failure
			return false;
		}
	}
	
	// handling the asterisk (matches one or more chars in path)
	if ((*(pattern+1) == '?') || (*(pattern+1) == '*'))
	{
		// stupid user/dev shenanigans, failure
		return false;
	}
	else if (*path == '\0')
	{
		// asterisk, but end reached on path, failure
		return false;
	}
	else if (*(pattern+1) == '\0')
	{
		// nothing after the asterisk, match found
		return true;
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
				if (cmp1->is_dir)
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
	FsFileInfo* finfo = (FsFileInfo*) malloc(N_DIR_READ * sizeof(FsFileInfo));
	s32 n_entries = 0;
	s32 dhandle;
	
	if (!finfo) // out of memory
		return -1;
	
	// set dir_buffer to all zeroes
	memset(dir_buffer, 0, sizeof(DirBufferEntry) * MAX_DIR_ENTRIES);
	
	// special handling when in root
	if(!*path)
	{
		const char* root_paths[] = { "sdmc:", "nand:", "twln:", "twlp:" };
		n_entries = sizeof(root_paths) / sizeof(const char*);
		
		for(s32 i = 0; i < n_entries; i++)
		{
			dir_buffer[i].fsize = 0;
			dir_buffer[i].is_dir = 1;
			dir_buffer[i].fname = mallocpyString(root_paths[i]);
		}
		
		return n_entries;
	}

	// open directory
	dhandle = fOpenDir(path);
	if(dhandle < 0)
	{
		free(finfo);
		return -1;
	}
	
	s32 n_read = 0;
	while((n_read = fReadDir(dhandle, finfo, N_DIR_READ)) > 0)
	{
		if (n_read < 0) // error reading dir
			goto fail;
		
		for(s32 i = 0; i < n_read; i++)
		{
			if(!(finfo[i].fattrib & AM_DIR) && pattern && !matchName(finfo[i].fname, pattern))
				continue; // not a match with the provided pattern and not a dir
			
			// take over data
			dir_buffer[n_entries].fsize = finfo[i].fsize;
			dir_buffer[n_entries].is_dir = finfo[i].fattrib & AM_DIR;
			dir_buffer[n_entries].fname = mallocpyString(finfo[i].fname);
			
			// check for out of memory
			if (!dir_buffer[n_entries].fname)
				goto fail;
			
			// max dir buffer size reached?
			if (++n_entries >= MAX_DIR_ENTRIES)
				break;
		}
		
		// max dir buffer size reached?
		if (n_entries >= MAX_DIR_ENTRIES)
			break; // WARNING: no errors here - list just isn't complete
	}
	
	fCloseDir(dhandle);
	free(finfo);
	
	if (n_entries > 0)
		sortDirBuffer(dir_buffer, n_entries);
	
	return n_entries;
	
	fail:
	
	fCloseDir(dhandle);
	free(finfo);
	
	return -1;	
}


/**
 * @brief Draws the file listing to the given console.
 * @param curr_path Current path displayed on screen.
 * @param dir_buffer An array of structs containing the file listing.
 * @param n_entries Number of elements in the above struct.
 * @param menu_con Console that the file browser is displayed on.
 * @param index Current placement of the cursor.
 * @param scroll Current scroll offset, will be written to by this function.
 */
void browserDraw(const char* curr_path, DirBufferEntry* dir_buffer, s32 n_entries, PrintConsole* menu_con, s32 index, s32* scroll)
{
	int brws_x = (menu_con->consoleWidth - BRWS_WIDTH) >> 1;
	int brws_y = BRWS_OFFSET_TITLE;
	char temp_str[BRWS_WIDTH + 1];
	char byte_str[32];
	
	// fix scroll (if required)
	if (index < *scroll)
	{
		*scroll = index;
	} else if (*scroll + BRWS_MAX_ENTRIES <= index)
	{
		*scroll = index - (BRWS_MAX_ENTRIES - 1);
	}
	
	// select menu console
	consoleSelect(menu_con);
	consoleClear();
	
	// browser title
	consoleSetCursor(menu_con, brws_x, brws_y++);
	truncateString(temp_str, curr_path, BRWS_WIDTH, 8);
	if (!*temp_str) strncpy(temp_str, "[root]", BRWS_WIDTH);
	ee_printf(temp_str);
	brws_y++;
	
	// menu entries
	for (s32 i = 0; i < BRWS_MAX_ENTRIES; i++)
	{
		s32 pos = i + *scroll;
		if (pos >= n_entries)
			break;
		
		DirBufferEntry* entry = &(dir_buffer[pos]);
		bool is_selected = (pos == index);
		
		if(entry->is_dir)
			strncpy(byte_str, *curr_path ? "(DIR)" : "(DRV)", 31);
		else
			formatBytes(byte_str, entry->fsize);
		
		consoleSetCursor(menu_con, brws_x, brws_y++);
		truncateString(temp_str, entry->fname, BRWS_WIDTH-13, 8);
		ee_printf(is_selected ? "\x1b[47;30m %-*.*s %10.10s \x1b[0m" : " %-*.*s %10.10s ",
			BRWS_WIDTH-13, BRWS_WIDTH-13, temp_str, byte_str);
	}
	
	// button instructions
	brws_y = BRWS_OFFSET_BUTTONS;
	consoleSetCursor(menu_con, brws_x, brws_y++);
	ee_printf("%-*.*s", BRWS_WIDTH, BRWS_WIDTH, "[A]:Choose [B]:Back");
}


/**
 * @brief Shows a file select dialogue.
 * @param res_path The selected path will end up here.
 * @param menu_con Console that the file browser is displayed on.
 * @param start Starting path (must be a dir).
 * @param patter Only files matching this wildcard pattern will be displayed.
 */
bool menuFileSelector(char* res_path, PrintConsole* menu_con, const char* start, const char* pattern)
{
	DirBufferEntry* dir_buffer;
	bool result = true; // <--- should be handled differently 
	
	dir_buffer = (DirBufferEntry*) malloc(MAX_DIR_ENTRIES * sizeof(DirBufferEntry));
	if (!dir_buffer) return false;
	
	// res_path has to be at least 256 byte long (including '\0') and
	// is also used as temporary buffer
	*res_path = '\0'; // root dir if start is NULL
	if(start) strncpy(res_path, start, 256);
	
	// check res_path, fix if required
	char* lastname = NULL;
	FsFileInfo fno;
	if (fStat(res_path, &fno) != FR_OK)
	{
		*res_path = '\0';
	}
	// not a dir -> must be a file
	else if (!(fno.fattrib & AM_DIR))
	{
		lastname = strrchr(res_path, '/');
		if (!lastname)
			panicMsg("Invalid path");
		*(lastname++) = '\0';
		
		s32 dhandle = fOpenDir(res_path);
		if (dhandle < 0)
			panicMsg("Filesystem error");
		fCloseDir(dhandle);
	}
	
	bool is_dir = true; // we are not finished while we have a dir in res_path
	while(is_dir)
	{
		s32 n_entries = readDirToBuffer(dir_buffer, res_path, pattern);
		s32 last_index = (u32) -1;
		s32 scroll = 0;
		s32 index = 0;
		
		if(n_entries < 0)
			panicMsg("Error reading dir!");
		
		// find lastname in listing
		if (lastname)
		{
			for (s32 i = 0; i < n_entries; i++)
			{
				if (strncmp(dir_buffer[i].fname, lastname, 256) == 0)
				{
					index = i;
					break;
				}
			}
			lastname = NULL;
		}
		
		while(true)
		{
			// update file browser (on demand)
			if (index != last_index) {
				browserDraw(res_path, dir_buffer, n_entries, menu_con, index, &scroll);
				last_index = index;

				GX_textureCopy((u64*)RENDERBUF_TOP, 0, (u64*)GFX_getFramebuffer(SCREEN_TOP),
							   0, SCREEN_SIZE_TOP + SCREEN_SIZE_SUB);
				GFX_swapFramebufs();
			}
			GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
			
			if(hidGetPowerButton(true)) // handle power button
			{
				power_off();
			}
			
			hidScanInput();
			const u32 kDown = hidKeysDown();
			const u32 kHeld = hidKeysHeld();
			
			if ((kDown & KEY_A) && n_entries)
			{
				// build new res_path
				char* name = &(res_path[strlen(res_path)]);
				if (name > res_path) *(name++) = '/';
				strncpy(name, dir_buffer[index].fname, (256 - (name - res_path)));
				
				// is this a dir?
				is_dir = dir_buffer[index].is_dir;
				
				lastname = NULL;
				break;
			}
			else if ((kDown & KEY_B) && *res_path)
			{
				// return to previous path
				// (the name of this path will be stored in lastname)
				lastname = strrchr(res_path, '/');
				
				if (lastname)
					*(lastname++) = '\0';
				else
					*res_path = '\0';
				break;
			}
			else if (kHeld & (KEY_DDOWN|KEY_DUP|KEY_DLEFT|KEY_DRIGHT) && n_entries)
			{
				if (kHeld & KEY_DDOWN)
				{
					// cursor down
					index = (index == n_entries - 1) ? 0 : index + 1;
				}
				else if (kHeld & KEY_DUP)
				{
					// cursor up
					index = (index == 0) ? n_entries - 1 : index - 1;
				}
				else if (kHeld & KEY_DRIGHT)
				{
					// cursor down a page
					index += BRWS_MAX_ENTRIES;
					if (index >= n_entries) index = n_entries - 1;
				}
				else if (kHeld & KEY_DLEFT)
				{
					// cursor up a page
					index -= BRWS_MAX_ENTRIES;
					if (index < 0) index = 0;
				}
				
				// sleep for 160ms
				TIMER_sleepMs(160);
			}
		}
		
		freeDirBufferContent(dir_buffer, n_entries);
	}
	
	free(dir_buffer);
	
	return result;
}