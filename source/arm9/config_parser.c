
/* This is a parser for the loadercfg.txt file. It's not vulnerable, I swear. */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "mem_map.h"
#include "arm9/fatfs/ff.h"
#include "arm9/console.h"
#include "arm9/main.h"
#include "arm9/interrupt.h"
#include "util.h"
#include "hid.h"

#define MAX_FILE_SIZE	0x4000 - 1

static const char *filepath = "sdmc:\\loadercfg.txt";

static FIL file;

typedef struct {
	char *textData;	// ptr to the first char of an attribute's data inside filebuf
	u32 textLength;		
	void *data;		// used to store the final native data
} AttributeEntryType;

typedef struct {
	// function to convert the textData to arbitrary native data
	bool (*parse)(AttributeEntryType *attr);
	// opposite of parse(), used to update/create an entry in the file
	bool (*write)(AttributeEntryType *attr);
} FunctionsEntryType;


static bool parseConfigFile();
static bool parseBootOption(AttributeEntryType *attr);
static bool parseBootOptionPad(AttributeEntryType *attr);
static bool parseBootMode(AttributeEntryType *attr);

enum Keys {
	KBootOption1 = 0,
	KBootOption2,
	KBootOption3,
	KBootOption1Buttons,
	KBootOption2Buttons,
	KBootOption3Buttons,
	KBootMode
};

#define numKeys  7

static const char *keyStrings[] = {
	"BOOT_OPTION1",
	"BOOT_OPTION2",
	"BOOT_OPTION3",
	"BOOT_OPTION1_BUTTONS",
	"BOOT_OPTION2_BUTTONS",
	"BOOT_OPTION3_BUTTONS",
	"BOOT_MODE"
};

static FunctionsEntryType keyFunctions[] = {
	{ parseBootOption,		NULL },
	{ parseBootOption,		NULL },
	{ parseBootOption,		NULL },
	{ parseBootOptionPad,	NULL },
	{ parseBootOptionPad,	NULL },
	{ parseBootOptionPad,	NULL },
	{ parseBootMode,		NULL }
};

static AttributeEntryType attributes[numKeys];

static char *filebuf = NULL;

/*	 */
bool loadConfigFile()
{
	FILINFO fileStat;
	u32 fileSize;
	unsigned bytesRead;
	
	if(f_stat(filepath, &fileStat) != FR_OK)
	{
		printf("Failed to get config-file status!\n");
		goto fail;
	}
	
	fileSize = fileStat.fsize;
	
	if(fileSize > MAX_FILE_SIZE || fileSize == 0)
	{
		printf("Invalid config-file size!\n");
		goto fail;
	}
	
	filebuf = (char *) malloc(fileSize + 1);
	
	if(!filebuf)
	{
		printf("Out of memory!\n");
		goto fail;
	}
	
	if(f_open(&file, filepath, FA_READ) != FR_OK)
	{
		printf("Failed to open config-file for reading!\n");
		goto fail;
	}
	
	if(f_read(&file, filebuf, fileSize, &bytesRead) != FR_OK || bytesRead != fileSize)
	{
		printf("Failed to read from config-file!\n");
		f_close(&file);
		goto fail;
	}
	
	// terminate string buf
	filebuf[fileSize] = '\0';
	
	
	return parseConfigFile();
	
fail:
	
	return false;
}

/* returns the start of an attribute's data. */
static char *findDefinition(const char *attrName)
{
	char *start;
	char c;
	
	start = strstr(filebuf, attrName);
	
	if(start)
	{
		// verify definition char
		start = start + strlen(attrName);
		c = *start;
		if(c != '=')
			start = NULL;
		else
			start ++;
	}
	
	return start;
}

/* returns the length of an attribute's data. */
static u32 parseDefinition(char *attrData)
{
	static const char LF = 0x0A, CR = 0x0D, NEL = 0x15;
	u32 len = 0;
	char *cur = attrData;
	char c;
	
	if(!attrData)
		return 0;
	
	// look for linebreaks that mark the end of a definition
	for(; (c = *cur) != '\0'; cur++)
	{
		if(c == LF || c == CR || c == NEL)
			break;
		len ++;
		c = *cur;
	}
	
	return len;
}

static bool parseConfigFile()
{
	AttributeEntryType *curAttr;
	u32 len;
	char *text;
	bool ret;
	
	// pass #1: look for definitions and populate our lookup table.
	for(u32 i=0; i<numKeys; i++)
	{
		curAttr = &attributes[i];
		text = findDefinition(keyStrings[i]);
		curAttr->textData = text;
		if(!text)
		{
			curAttr->textLength = 0;
			continue;
		}
		len = parseDefinition(text);
		curAttr->textLength = len;
	}
	
	// pass #2: parse text-data and convert it into a native form
	for(u32 i=0; i<numKeys; i++)
	{
		curAttr = &attributes[i];
		if(!curAttr->textData)
		{
			curAttr->data = NULL;
			continue;
		}
		else
		{	// TODO: what to do if ret == false ?
			ret = keyFunctions[i].parse(curAttr);
		}
	}
	
	return true;
}

static bool isValidPath(char *path)
{
	char c;
	bool gotMountpoint = false;
	
	for(; (c = *path) != '\0'; path++)
	{
		// no multiple mountpoints
		if(c == ':')
		{
			if(gotMountpoint)
				return false;
			else
				gotMountpoint = true;
		}
		
		// no dir-return
		if(c == '.' && path[1] == '.')
			return false;
			
		// no non ASCII chars
		if(!isascii(c))
			return false;
	}
	
	if(!gotMountpoint)
		return false;
		
	return true;
}

static bool parseBootOption(AttributeEntryType *attr)
{
	attr->data = NULL;
	
	if(!isValidPath(attr->textData))
		return false;
		
	attr->data = strdup(attr->textData);
	if(!attr->data) return false;
	
	return true;
}

static bool parseBootOptionPad(AttributeEntryType *attr)
{
	char c;
	char *textData = attr->textData;
	u32 padValue = 0;
	u32 i = 0;
	
	static const char * convTable[] = {
		"A", "B", "SELECT", "START", "RIGHT", "LEFT",
		"UP", "DOWN", "R", "L", "X", "Y"
	};
	
	for(; (c = *textData) != '\0'; textData ++)
	{
		if(strnicmp(textData, convTable[i], strlen(convTable[i])) == 0)
		{
			padValue |= 1 << i;
			i ++;
			textData += strlen(convTable[i]);
		}
	}
	
	attr->data = (u32 *) malloc(sizeof(u32));
	if(!attr->data)
		return false;
		
	*(u32 *)attr->data = padValue;
	return true;
}

static bool parseBootMode(AttributeEntryType *attr)
{
	// TODO
	attr->data = NULL;
	return true;
}
