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
#include "arm11/menu/menu_util.h"
#include "arm11/menu/menu_color.h"
#include "arm11/hardware/hid.h"
#include "arm11/hardware/timer.h"
#include "arm11/config.h"
#include "arm11/console.h"
#include "arm11/debug.h"
#include "arm11/fmt.h"

#define MAX_DIR_ENTRIES	0x100 // 256 -> worst case approx 64kiB (yes, this is limited)
#define N_DIR_READ		0x10  // 16 at a time

typedef struct {
	u32 fsize;		// size of the file
	u8  is_dir;		// > 0 if is directory
	char* fname;	// filename (handle via malloc)
} DirBufferEntry;



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
	
	// handling the asterisk (matches any # of chars in path)
	if ((*(pattern+1) == '?') || (*(pattern+1) == '*'))
	{
		// stupid user/dev shenanigans, failure
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
		for (; *path != '\0'; path++)
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
		dir_buffer[i].fname = NULL;
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
			if(strnicmp(min0->fname, cmp1->fname, FF_MAX_LFN + 1) > 0)
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
	s32 n_entries = 0;
	
	// set dir_buffer to all zeroes
	memset(dir_buffer, 0, sizeof(DirBufferEntry) * MAX_DIR_ENTRIES);
	
	// special handling when in root
	if(!*path)
	{
		const char* root_paths[] = { "sdmc:", "twln:", "twlp:", "nand:" };
		const char* firm_paths[] = { "firm1:" };
		const u32 firm_size = 0x400000; // 4MB
		bool devmode = configDataExist(KDevMode) && (*(bool*) configGetData(KDevMode));
		
		for(u32 i = 0; i < sizeof(root_paths) / sizeof(const char*); i++)
		{
			if (!fsEnsureMounted(root_paths[i]))
			 	continue;
			
			dir_buffer[n_entries].fsize = 0;
			dir_buffer[n_entries].is_dir = 1;
			dir_buffer[n_entries].fname = mallocpyString(root_paths[i]);
			n_entries++;
		}
		
		if(devmode)
		{
			for(u32 i = 0; i < sizeof(firm_paths) / sizeof(const char*); i++)
			{
				if (!matchName(firm_paths[i], pattern))
					continue;
				
				dir_buffer[n_entries].fsize = firm_size;
				dir_buffer[n_entries].is_dir = 0;
				dir_buffer[n_entries].fname = mallocpyString(firm_paths[i]);
				n_entries++;
			}
		}
		
		return n_entries;
	}

	// open directory
	s32 dhandle = fOpenDir(path);
	if(dhandle < 0) return -1;
    
	FsFileInfo* finfo = (FsFileInfo*) malloc(N_DIR_READ * sizeof(FsFileInfo));
	if (!finfo) // out of memory
	{
		fCloseDir(dhandle);
		return -1; 
	}
        
	s32 n_read = 0;
	while((n_read = fReadDir(dhandle, finfo, N_DIR_READ)) != 0)
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
	truncateString(temp_str, *curr_path ? curr_path : "root", BRWS_WIDTH, 8);
	ee_printf(ESC_SCHEME_ACCENT1 "%-*s" ESC_RESET, BRWS_WIDTH, temp_str);
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
		ee_printf(entry->is_dir ? ESC_SCHEME_WEAK : ESC_SCHEME_STD);
		if (is_selected)ee_printf(ESC_INVERT);
		ee_printf(" %-*.*s %10.10s ", BRWS_WIDTH-13, BRWS_WIDTH-13, temp_str, byte_str);
		ee_printf(ESC_RESET);
	}
	
	// button instructions
	brws_y = BRWS_OFFSET_BUTTONS;
	ee_printf(ESC_SCHEME_WEAK);
	consoleSetCursor(menu_con, brws_x, brws_y++);
	ee_printf("%-*.*s", BRWS_WIDTH, BRWS_WIDTH, "[A]:Choose [B]:Back");
	consoleSetCursor(menu_con, brws_x, brws_y++);
	ee_printf("%-*.*s", BRWS_WIDTH, BRWS_WIDTH, "[HOME] to cancel");
	ee_printf(ESC_RESET);
}


/**
 * @brief Shows a file select dialogue.
 * @param res_path The selected path will end up here.
 * @param menu_con Console that the file browser is displayed on.
 * @param start Starting path (must be a dir).
 * @param pattern Only files matching this wildcard pattern will be displayed.
 */
bool menuFileSelector(char* res_path, PrintConsole* menu_con, const char* start, const char* pattern, bool allow_root)
{
	DirBufferEntry* dir_buffer;
	bool result = true; // <--- should be handled differently 
	
	dir_buffer = (DirBufferEntry*) malloc(MAX_DIR_ENTRIES * sizeof(DirBufferEntry));
	if (!dir_buffer) return false;
	
	// res_path has to be at least 256 byte long (including '\0') and
	// is also used as temporary buffer
	*res_path = '\0'; // root dir if start is NULL
	if(start) strncpy(res_path, start, FF_MAX_LFN + 1);
    
    // handle allow_root
    if (!allow_root && (strncmp(res_path, "sdmc:", 5) != 0))
         strncpy(res_path, "sdmc:", FF_MAX_LFN + 1);
	
	// check res_path, fix if required
	char* lastname = NULL;
	s32 dhandle;
	// is this a dir?
	if ((dhandle = fOpenDir(res_path)) >= 0)
	{
		fCloseDir(dhandle);
	}
	// not a dir -> must be a file
	else if (fStat(res_path, NULL) == FR_OK)
	{
		lastname = strrchr(res_path, '/');
		if (!lastname)
			panicMsg("Invalid path");
		*(lastname++) = '\0';
		
		dhandle = fOpenDir(res_path);
		if (dhandle < 0)
			panicMsg("Filesystem corruption");
		fCloseDir(dhandle);
	}
	// neither file nor dir
	else
	{
		*res_path = '\0';
	}
	
	bool is_dir = true; // we are not finished while we have a dir in res_path
	while(is_dir && result)
	{
		s32 n_entries = readDirToBuffer(dir_buffer, res_path, pattern);
		s32 last_index = (u32) -1;
		s32 scroll = 0;
		s32 index = 0;
		
		if (n_entries < 0)
		{
			if (*res_path)
			{
				// return to root if error encountered
				*res_path = '\0';
				lastname = NULL;
				continue;
			} else panicMsg("Root filesystem failure!");
		}
		
		// find lastname in listing
		if (lastname)
		{
			for (s32 i = 0; i < n_entries; i++)
			{
				if (strncmp(dir_buffer[i].fname, lastname, FF_MAX_LFN + 1) == 0)
				{
					index = i;
					break;
				}
			}
			lastname = NULL;
		}
		
		u32 dbutton_cooldown = 0;
		while(result)
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
			
			// directional button cooldown
			for (u32 i = dbutton_cooldown; i > 0; i--)
			{
				hidScanInput();
				GFX_waitForEvent(GFX_EVENT_PDC0, true); // VBlank
				if (!(hidKeysHeld() & (KEY_DDOWN|KEY_DUP|KEY_DLEFT|KEY_DRIGHT)))
					break;
			}
			dbutton_cooldown = 0;
			
			// handle power button
			if(hidGetPowerButton(false))
				result = false;
			
			hidScanInput();
			const u32 kDown = hidKeysDown();
			const u32 kHeld = hidKeysHeld();
			
			if (kDown & KEY_SHELL)
			{
				sleepmode();
			}
			else if (kDown & KEY_HOME)
			{
				result = false;
			}
			else if ((kDown & KEY_A) && n_entries)
			{
				// build new res_path
				char* name = &(res_path[strlen(res_path)]);
				if (name > res_path) *(name++) = '/';
				strncpy(name, dir_buffer[index].fname, ((FF_MAX_LFN + 1) - (name - res_path)));
				
				// is this a dir?
				is_dir = dir_buffer[index].is_dir;
				
				lastname = NULL;
				break;
			}
			else if (kDown & KEY_B)
			{
				// if in root: return to menu
				if (!*res_path)
				{
					result = false;
					break;
				}
				
				// if not in root: return to previous path
				// (the name of this path will be stored in lastname)
				lastname = strrchr(res_path, '/');
				
				if (lastname)
					*(lastname++) = '\0';
				else if (allow_root)
					*res_path = '\0';
				else
					result = false;
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
				
				// set directional button cooldown
				dbutton_cooldown = 10;
			}
		}
		
		freeDirBufferContent(dir_buffer, n_entries);
	}
	
	free(dir_buffer);
	
	return result;
}