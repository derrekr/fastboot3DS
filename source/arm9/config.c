
/* This is a parser for the loadercfg.txt file. It's not vulnerable, I swear. */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "mem_map.h"
#include "fatfs/ff.h"
#include "arm9/console.h"
#include "arm9/main.h"
#include "arm9/interrupt.h"
#include "util.h"
#include "hid.h"
#include "arm9/config.h"

#define MAX_FILE_SIZE	0x4000 - 1

static const char *filepath = "sdmc:\\loadercfg.txt";

static FIL file;

typedef struct {
	char *textData;	// ptr to the first char of an attribute's data inside filebuf
	u32 textLength;	// length of the above data, without '\0' ofc
	void *data;		// used to store the final native data
} AttributeEntryType;

typedef struct {
	// function to convert the textData to arbitrary native data
	bool (*parse)(AttributeEntryType *attr);
	// opposite of parse(), used to update/create an entry in the file
	bool (*write)(AttributeEntryType *attr, void *newData, int key);
} FunctionsEntryType;


static bool parseConfigFile();
static bool parseBootOption(AttributeEntryType *attr);
static bool writeBootOption(AttributeEntryType *attr, void *newData, int key);
static bool parseBootOptionPad(AttributeEntryType *attr);
static bool parseBootMode(AttributeEntryType *attr);
static bool writeBootMode(AttributeEntryType *attr, void *newData, int key);

static const char *keyStrings[] = {
	"BOOT_OPTION1",
	"BOOT_OPTION2",
	"BOOT_OPTION3",
	"BOOT_OPTION1_NAND_IMAGE",
	"BOOT_OPTION2_NAND_IMAGE",
	"BOOT_OPTION3_NAND_IMAGE",
	"BOOT_OPTION1_BUTTONS",
	"BOOT_OPTION2_BUTTONS",
	"BOOT_OPTION3_BUTTONS",
	"BOOT_MODE"
};

static FunctionsEntryType keyFunctions[] = {
	{ parseBootOption,		writeBootOption },
	{ parseBootOption,		writeBootOption },
	{ parseBootOption,		writeBootOption },
	/* use the same functions for nand image option */
	{ parseBootOption,		writeBootOption },
	{ parseBootOption,		writeBootOption },
	{ parseBootOption,		writeBootOption },
	{ parseBootOptionPad,	NULL },
	{ parseBootOptionPad,	NULL },
	{ parseBootOptionPad,	NULL },
	{ parseBootMode,		writeBootMode }
};

static AttributeEntryType attributes[numKeys];

static char *filebuf = NULL;

/* This loads the config file from SD card and parses it */
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
	
	filebuf = (char *) malloc(MAX_FILE_SIZE + 1);
	
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
	
	f_close(&file);
	
	// terminate string buf
	filebuf[fileSize] = '\0';
	
	return parseConfigFile();
	
fail:
	
	return false;
}

static bool writeConfigFile()
{
	const u32 fileSize = strlen(filebuf);
	u32 bytesWritten;
	
	if(fileSize > MAX_FILE_SIZE)
		panic();
	
	if(f_open(&file, filepath, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		goto fail;
	}
	
	if((f_write(&file, filebuf, fileSize, &bytesWritten) != FR_OK) || (bytesWritten != fileSize))
	{
		f_close(&file);
		goto fail;
	}
	
	f_close(&file);
	
	return true;
	
fail:

	return false;
}

bool createConfigFile()
{
	filebuf = (char *) malloc(MAX_FILE_SIZE + 1);
	
	if(!filebuf)
		return false;
	
	filebuf[0] = 0x0d;
	filebuf[1] = 0x0a;
	filebuf[2] = 0x00;
	
	return writeConfigFile();
}

/* returns the start of an attribute's data. */
static char *findDefinition(const char *attrName)
{
	char *start;
	char c;
	
	start = strstr(filebuf, attrName);
	
	if(start)
	{
		/* verify definition char */
		start = start + strlen(attrName);
		
		while(*start == ' ') start++;
		
		c = *start;
		if(c != '=')
			start = NULL;
		else
		{
			start ++;
			while(*start == ' ') start++;
		}
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
			//printf("Parsing attr text with size %i\n%s\n", curAttr->textLength, curAttr->textData);
			keyFunctions[i].parse(curAttr);
		}
	}
	
	return true;
}

// Add a new definition in our file
static char *writeAddDefinitionText(const char *keyName, const char *textData)
{
	const char *def = " = ";
	u32 curLen = strlen(filebuf);
	u32 remainingLen = MAX_FILE_SIZE - curLen;
	u32 totalLen = strlen(textData);
	u32 defLen = strlen(def);
	totalLen += strlen(keyName);
	totalLen += defLen;
	totalLen += 2;	// new line encoding
	
	if(totalLen > remainingLen)
		return NULL;
	
	filebuf[curLen++] = 0x0d;
	filebuf[curLen++] = 0x0a;
	sprintf(&filebuf[curLen], "%s%s%s", keyName, def, textData);
	
	return &filebuf[curLen + defLen];
}

static u32 writeUpdateDefinitionText(char *textData, u32 curTextLen, const char *newText)
{
	u32 diff;
	u32 newLen = strlen(newText);
	u32 curLen = strlen(filebuf);
	u32 remainingLen = MAX_FILE_SIZE - curLen;
	
	if(newLen > remainingLen)
		return 0;
	
	diff = newLen - curTextLen;
	
	if(diff)
	{
		if(curTextLen < newLen)
			memcpy(textData + diff, textData + curTextLen,
						curLen - (filebuf - textData + diff) + 1);
		else
			memcpy(textData + newLen, textData + curTextLen,
						curLen - (filebuf - textData + curTextLen) + 1);
	
		for(u32 key=0; key<numKeys; key++)
		{
			if(attributes[key].textData > textData)
			{
				attributes[key].textData += diff;
			}
		}
	}
	
	memcpy(textData, newText, newLen);
	
	return newLen;
}

static void writeAttributeText(AttributeEntryType *attr, const char *newText, int key)
{
	// text data doesn't exist yet?
	if(!attr->textData)
	{
		attr->textData = writeAddDefinitionText(configGetKeyText(key), newText);
		attr->textLength = strlen(newText);
	}
	else	// update definition
	{
		attr->textLength = writeUpdateDefinitionText(attr->textData, attr->textLength, newText);
	}
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

		// no space after dir slash
		if((c == '\\' || c == '/') && path[1] == ' ')
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
	
	char *buf = (char *) malloc(attr->textLength + 1);
	if(!buf) return false;
	
	memcpy(buf, attr->textData, attr->textLength);
	buf[attr->textLength] = '\0';
	
	if(!isValidPath(buf))
	{
		free(buf);
		return false;
	}
	
	attr->data = buf;
	
	return true;
}

static bool writeBootOption(AttributeEntryType *attr, void *newData, int key)
{
	u32 len;
	char *buf;
	const char *path = (const char *) newData;

	if(!path)
		return false;
	
	if(!isValidPath(path))
		return false;
	
	len = strlen(path);
	
	if(!attr->data)
	{
		buf = (char *) malloc(len + 1);
		if(!attr->data)
			return false;
	}
	else if(len != attr->textLength)
	{
		free(attr->data);
		buf = (char *) malloc(len + 1);
		if(!attr->data)
			return false;
	}
	
	memcpy(buf, path, len);
	buf[len] = '\0';
	
	attr->data = buf;
	attr->textLength = len;
	
	writeAttributeText(attr, path, key);
	
	return true;
}

static bool parseBootOptionPad(AttributeEntryType *attr)
{
	char c;
	char *textData = attr->textData;
	u32 padValue = 0;
	u32 i;
	
	static const char * convTable[] = {
		"A", "B", "SELECT", "START", "RIGHT", "LEFT",
		"UP", "DOWN", "R", "L", "X", "Y"
	};
	
	for(; (c = *textData) != '\0' && textData < attr->textData + attr->textLength;)
	{
		for(i=0; i<arrayEntries(convTable); i++)
		{
			if(strnicmp(textData, convTable[i], strlen(convTable[i])) == 0)
			{
				padValue |= (u32) 1 << i;
				break;
			}
		}
		if(i == arrayEntries(convTable))
			textData ++;
		else textData += strlen(convTable[i]);
	}
	
	attr->data = (u32 *) malloc(sizeof(u32));
	if(!attr->data)
		return false;
		
	*(u32 *)attr->data = padValue;
	return true;
}

static const char * modeTable[] = {
	"Normal", "Quick", "Quiet"
};

static bool parseBootMode(AttributeEntryType *attr)
{
	char *textData = attr->textData;
	u32 i;

	for(i=0; i<3; i++)
	{
		if(strnicmp(textData, modeTable[i], strlen(modeTable[i])) == 0)
			break;
	}

	if(i >= 3)
	{
		attr->data = NULL;
		return false;
	}

	attr->data = (u32 *) malloc(sizeof(u32));
	if(!attr->data)
		return false;

	*(u32 *)attr->data = i;
	return true;
}

static bool writeBootMode(AttributeEntryType *attr, void *newData, int key)
{
	u32 mode;
	
	if(!newData)
		return false;
	
	mode = *(u32 *)newData;
	
	if(mode > BootModeQuiet)
		return false;
	
	if(!attr->data)
	{
		attr->data = (u32 *) malloc(sizeof(u32));
		if(!attr->data)
			return false;
	}
	
	*(u32 *)attr->data = mode;
	
	const char *data = modeTable[mode];
	
	writeAttributeText(attr, data, key);
	
	return true;
}

void *configCopyText(int key)
{
	if(key < 0 || key >= KLast)
		return NULL;
	
	if(!attributes[key].textData)
		return NULL;
	
	char *buf = (char *) malloc(attributes[key].textLength + 1);
	if(!buf)
		return NULL;
	
	memcpy(buf, attributes[key].textData, attributes[key].textLength);
	buf[attributes[key].textLength] = '\0';
	
	return buf;
}

const void *configGetData(int key)
{
	if(key < 0 || key >= KLast)
		return NULL;
	
	return attributes[key].data;
}

const char *configGetKeyText(int key)
{
	if(key < 0 || key >= KLast)
		return NULL;
	
	return keyStrings[key];
}

bool configSetKeyData(int key, void *data)
{
	AttributeEntryType *attr;
	
	if(key < 0 || key >= KLast)
		return NULL;
	
	attr = &attributes[key];
	
	if(!keyFunctions[key].write)
		panic();
	
	return keyFunctions[key].write(attr, data, key);
}

void configRestoreDefaults()
{
	for(int key=0; key<numKeys; key++)
	{
		if(key == KBootMode)
			configSetKeyData(KBootMode, "Normal");
		else
			configSetKeyData(key, "");
	}
}