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

/* This is a parser/writer for the fastbootcfg.txt file. It's not vulnerable, I swear. */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "mem_map.h"
#include "util.h"
#include "fsutils.h"
#include "arm11/hardware/hid.h"
#include "arm11/debug.h"
#include "arm11/config.h"
#include "arm11/fmt.h"

#define MAX_FILE_SIZE	(0x4000u - 1)

static const char *SdmcFilepath = "sdmc:/3ds/fastbootcfg.txt";
static const char *NandFilepath = "nand:/fastboot3DS/fastbootcfg.txt";

static const char *filepath;

typedef struct {
	char *textData;	// ptr to the first char of an attribute's data inside filebuf
	u32 textLength;	// length of the above data, without '\0' ofc
	void *data;		// used to store the final native data
} AttributeEntryType;

typedef struct {
	// function to convert the textData to arbitrary native data
	bool (*parse)(AttributeEntryType *attr);
	// opposite of parse(), used to update/create an entry in the file
	bool (*write)(AttributeEntryType *attr, const void *newData, int key);
} FunctionsEntryType;

static void unloadConfigFile();
static bool createConfigFile();
static bool parseConfigFile();
static bool parsePath(AttributeEntryType *attr);
static bool writePath(AttributeEntryType *attr, const void *newData, int key);
static bool parseBootOptionPad(AttributeEntryType *attr);
static bool writeBootOptionPad(AttributeEntryType *attr, const void *newData, int key);
static bool parseBootMode(AttributeEntryType *attr);
static bool writeBootMode(AttributeEntryType *attr, const void *newData, int key);
static bool parseBool(AttributeEntryType *attr);
static bool writeBool(AttributeEntryType *attr, const void *newData, int key);
static bool parseInt(AttributeEntryType *attr);
static bool writeInt(AttributeEntryType *attr, const void *newData, int key);

static const char *keyStrings[] = {
	"BOOT_OPTION1",
	"BOOT_OPTION2",
	"BOOT_OPTION3",
	"BOOT_OPTION4",
	"BOOT_OPTION5",
	"BOOT_OPTION6",
	"BOOT_OPTION7",
	"BOOT_OPTION8",
	"BOOT_OPTION9",
	
	"SPLASH_SCREEN",

	"BOOT_OPTION1_BUTTONS",
	"BOOT_OPTION2_BUTTONS",
	"BOOT_OPTION3_BUTTONS",
	"BOOT_OPTION4_BUTTONS",
	"BOOT_OPTION5_BUTTONS",
	"BOOT_OPTION6_BUTTONS",
	"BOOT_OPTION7_BUTTONS",
	"BOOT_OPTION8_BUTTONS",
	"BOOT_OPTION9_BUTTONS",

	"BOOT_MODE",
	"DEV_MODE",
	"RAM_FIRM_BOOT",

	"SPLASH_DURATION"
	
	/*
	"BOOT_OPTION1_NAND_IMAGE",
	"BOOT_OPTION2_NAND_IMAGE",
	"BOOT_OPTION3_NAND_IMAGE",
	*/
};

static AttributeEntryType attributes[numKeys];

static char *filebuf = NULL;

static bool configLoaded = false;
static bool configDirty = false;

static const FunctionsEntryType *getKeyFunctions(int key)
{
	static const FunctionsEntryType keyFunctions[] = {
		{ parsePath,			writePath },
		{ parseBootOptionPad,	writeBootOptionPad },
		{ parseBootMode,		writeBootMode },
		{ parseBool,			writeBool },
		{ parseInt,				writeInt }
	};
	
	if(key <= KSplashScreen && key >= KBootOption1)
		return &keyFunctions[0];
	if(key <= KBootOption9Buttons && key >= KBootOption1Buttons)
		return &keyFunctions[1];
	if(key == KBootMode)
		return &keyFunctions[2];
	if(key == KDevMode || key == KRamFirmBoot)
		return &keyFunctions[3];
	if(key == KSplashDuration)
		return &keyFunctions[4];

	return NULL;
}

/* This loads the config file from SD card or eMMC and parses it */
bool loadConfigFile()
{
	FILINFO fileStat;
	s32 file;
	u32 fileSize;
	bool SdPresent;
	bool adpotChanges = false;
	bool createFile = false;
	
	if(filebuf)
		unloadConfigFile();
	
	configDirty = false;
	
	
	/* first, try SD card fatfs */
	
	SdPresent = fIsDevActive(FS_DEVICE_SDMC);
	
	if(SdPresent)
	{
		filepath = SdmcFilepath;
	
		// does the config file not exist yet?
		if(fStat(SdmcFilepath, &fileStat) != FR_OK)
		{
			if(fStat(NandFilepath, &fileStat) == FR_OK)
			{
				/*	no config on SD, but there is one on the NAND,
					so just load it and save it later on.	*/
				adpotChanges = true;
			}
			
			createFile = true;
		}	
	}
	else	/* use NAND */
	{
		filepath = NandFilepath;
		
		if(fStat(filepath, &fileStat) != FR_OK)
		{
			filepath = NandFilepath;
		}
	}
	
	if(createFile)
	{
		// try to create a file
		if(!createConfigFile())
			return false;
		
		if(adpotChanges)
		{
			// created file on SD, load config from NAND
			filepath = NandFilepath;
		}
		
		// retrieve size
		if(fStat(filepath, &fileStat) != FR_OK)
			return false;
	}
	
	fileSize = fileStat.fsize;
	
	if(fileSize > MAX_FILE_SIZE)
	{
		//ee_printf("Invalid config-file size!\n");
		return false;
	}
	
	filebuf = (char *) malloc(MAX_FILE_SIZE + 1);
	
	if(!filebuf)
	{
		//ee_printf("Out of memory!\n");
		return false;
	}
	
	if(fileSize)
	{
		if((file = fOpen(filepath, FS_OPEN_READ)) < 0)
		{
			//ee_printf("Failed to open config-file for reading!\n");
			goto fail;
		}
		
		if(fRead(file, filebuf, fileSize) != FR_OK)
		{
			//ee_printf("Failed to read from config-file!\n");
			fClose(file);
			goto fail;
		}
		
		fClose(file);
	}
	
	// terminate string buf
	filebuf[fileSize] = '\0';
	
	if(!parseConfigFile())
		goto fail;
	
	// we loaded a file we want to adpot?
	if(adpotChanges)
	{
		filepath = SdmcFilepath;
		configDirty = true;
	}
	
	return true;
	
fail:
	
	// something didn't work, clean everything up
	unloadConfigFile();
	
	return false;
}

static void unloadConfigFile()
{
	configLoaded = false;
	AttributeEntryType *curAttr;
	
	/* Free all data */
	for(u32 i=0; i<numKeys; i++)
	{
		curAttr = &attributes[i];
		
		curAttr->textData = NULL;
		curAttr->textLength = 0;
			
		if(curAttr->data)
		{
			free(curAttr->data);
			curAttr->data = NULL;
		}
	}
	
	if(filebuf)
	{
		free(filebuf);
		filebuf = NULL;
	}
}

bool configIsLoaded()
{
	return configLoaded;
}

bool configHasChanged()
{
	return configDirty;
}

FsDevice configGetStorageLocation()
{
	if(!configLoaded)
		panic();

	if(filepath == SdmcFilepath)
		return FS_DEVICE_SDMC;
	
	return FS_DEVICE_NAND;
}

bool writeConfigFile()
{
	s32 file;
	
	if(!filebuf)
		goto fail;

	const u32 fileSize = strlen(filebuf);
	
	if(fileSize > MAX_FILE_SIZE)
		panicMsg("fileSize too large!");
	
	if(!fsCreateFileWithPath(filepath))
	{
		goto fail;
	}
	
	if((file = fOpen(filepath, FS_OPEN_WRITE)) < 0)
	{
		goto fail;
	}
	
	if(fileSize)
	{
		if(fWrite(file, filebuf, fileSize) != 0)
		{
			fClose(file);
			goto fail;
		}
	}

	// Make sure changes are written to disk in case
	// the SD card is removed later.
	if(fSync(file) != FR_OK)
	{
		fClose(file);
		goto fail;
	}

	fClose(file);
	
	return true;
	
fail:

	return false;
}

static bool createConfigFile()
{
	// set default settings for new config files
	const char *temp = "BOOT_MODE = Normal\r\n";
	bool ret;
	
	if(filebuf)
		panicMsg("config: internal error");
	
	filebuf = (char*)temp;
	
	ret = writeConfigFile();
	
	filebuf = NULL;
	
	return ret;
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

static inline bool isEOL(const char c)
{
	const char LF = 0x0A, CR = 0x0D, NEL = 0x15;
	
	if(c == LF || c == CR || c == NEL)
		return true;
	
	return false;
}

/* returns the length of an attribute's data. */
static u32 parseDefinition(char *attrData)
{
	u32 len = 0;
	char *cur = attrData;
	char c;
	
	if(!attrData)
		return 0;
	
	// look for linebreaks that mark the end of a definition
	for(; (c = *cur) != '\0'; cur++)
	{
		if(isEOL(c))
			break;
		len ++;
		c = *cur;
	}
	
	return len;
}

static bool parseConfigFile()
{
	AttributeEntryType *curAttr;
	const FunctionsEntryType *keyFunc;
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
		{
			keyFunc = getKeyFunctions(i);
			if(keyFunc && keyFunc->parse)
				keyFunc->parse(curAttr);
			else curAttr->data = NULL;
		}
	}
	
	configLoaded = true;
	
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
	u32 keyLen = strlen(keyName);
	
	if(totalLen > remainingLen)
		return NULL;
	
	totalLen += keyLen;
	totalLen += defLen;
	
	if(totalLen > remainingLen)
		return NULL;
	
	totalLen += 2;	// size of new line encoding
	
	if(totalLen > remainingLen)
		return NULL;
	
	/* if there's no linebreak at the end already, add one */
	if(curLen != 0 && !isEOL(filebuf[curLen - 1]))
	{
		totalLen += 2;
		
		if(totalLen > remainingLen)
			return NULL;
		
		if(curLen != 0)
		{
			filebuf[curLen++] = 0x0d;
			filebuf[curLen++] = 0x0a;
		}
	}
	
	/* format print line and terminate it */
	ee_sprintf(&filebuf[curLen], "%s%s%s\r\n", keyName, def, textData);
	
	return &filebuf[curLen + keyLen + defLen];
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
		if(newLen > curTextLen)
		{
			size_t backupLen = curLen - (textData - filebuf) - curTextLen + 1;
			
			char *tempBuf = (char *) malloc(backupLen);
			if(!tempBuf)
				panicMsg("writeUpdateDefinitionText OOM!");
			
			memcpy(tempBuf, textData + curTextLen, backupLen);
			
			memcpy_s(filebuf, MAX_FILE_SIZE + 1, textData - filebuf + newLen,
						tempBuf, backupLen, 0, false);
			free(tempBuf);
		}
		else
		{
			memcpy_s(filebuf, MAX_FILE_SIZE + 1, textData - filebuf + newLen,
						filebuf, MAX_FILE_SIZE + 1, textData - filebuf + curTextLen, false);
		}
		
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

static bool removeDefinition(AttributeEntryType *attr)
{
	const char *attrText = attr->textData;
	const size_t attrLength = attr->textLength;
	char *p = (char *)attrText;
	char *lineBegin;
	char *lineEnd;
	char c;
	
	if(!attrText || !attrLength)
		return false;
	
	/* go back and find the beginning of the key string */
	
	do
	{
		c = *p--;
		if(isEOL(c))
			break;
	} while(p > filebuf);
	
	if(isEOL(c))
		lineBegin = p + 2;
	else
	{
		if(p != filebuf) panicMsg("removeDefinition BUG!");
		lineBegin = p;
	}
	
	/* find the end of this line */
	
	p = (char *)attrText + attrLength;
	
	do
	{
		c = *p++;
		if(isEOL(c))
			break;
	} while(c);
	
	if(c == '\0')
		lineEnd = p - 1;
	else
	{
		if(isEOL(*p))
			lineEnd = p + 1;
		else lineEnd = p;
	}
	
	if(writeUpdateDefinitionText(lineBegin, lineEnd - lineBegin, "") != 0)
		return false;
	
	return true;
}

static bool isValidPath(const char *path)
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
			gotMountpoint = true;
		}
		
		// no dir-return
		if(c == '.' && path[1] == '.')
			return false;

		// no space after dir slash
		if((c == '\\' || c == '/') && path[1] == ' ')
			return false;
			
		// no non ASCII chars
		if(!((unsigned char)c <= 127))
			return false;
	}
	
	if(!gotMountpoint)
		return false;
		
	return true;
}

static bool parsePath(AttributeEntryType *attr)
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

static bool writePath(AttributeEntryType *attr, const void *newData, int key)
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
		if(!buf)
			return false;
		attr->data = buf;
	}
	else if(len != attr->textLength)
	{
		attr->data = realloc(attr->data, len + 1);
		buf = (char *)attr->data;
		if(!buf)
			return false;
	}
	else buf = attr->data;
	
	memcpy(buf, path, len);
	buf[len] = '\0';
	
	writeAttributeText(attr, path, key);
	
	return true;
}

static const char * convTable[] = {
	"A", "B", "SELECT", "START", "RIGHT", "LEFT",
	"UP", "DOWN", "R", "L", "X", "Y"
};

static bool parseBootOptionPad(AttributeEntryType *attr)
{
	char c;
	char *textData = attr->textData;
	u32 padValue = 0;
	u32 i;
	
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

static bool writeBootOptionPad(AttributeEntryType *attr, const void *newData, int key)
{
	char tempbuf[0x40] = { 0 };
	u32 padValue;
	u32 i;
	bool keysAdded = false;
	
	if(!newData)
		return false;
	
	padValue = *(const u32 *)newData;
	
	padValue &= HID_KEY_MASK_ALL;
	
	if(!attr->data)
	{
		attr->data = (u32 *) malloc(sizeof(u32));
		if(!attr->data)
			return false;
	}
	
	*(u32 *)attr->data = padValue;
	
	for(i=0; i<arrayEntries(convTable); i++)
	{
		if((padValue >> i) & 1)
		{
			if(keysAdded)
				strcat(tempbuf, " + ");
			strcat(tempbuf, convTable[i]);
			keysAdded = true;
		}
	}
	
	writeAttributeText(attr, tempbuf, key);
	
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

static bool writeBootMode(AttributeEntryType *attr, const void *newData, int key)
{
	u32 mode;
	
	if(!newData)
		return false;
	
	mode = *(const u32 *)newData;
	
	if(mode > BootModeQuiet)
		return false;
	
	if(!attr->data)
	{
		attr->data = (u32 *) malloc(sizeof(u32));
		if(!attr->data)
			return false;
	}
	
	*(u32 *)attr->data = mode;
	
	const char *textData = modeTable[mode];
	
	writeAttributeText(attr, textData, key);
	
	return true;
}

static const char * boolStates[] = {
	"Enabled", "Disabled"
};

static bool parseBool(AttributeEntryType *attr)
{
	char *textData = attr->textData;
	bool enabled;

	if(strnicmp(textData, boolStates[0], strlen(boolStates[0])) == 0)
		enabled = true;
	else if(strnicmp(textData, boolStates[1],  strlen(boolStates[1])) == 0)
		enabled = false;
	else
	{
		attr->data = NULL;
		return false;
	}

	attr->data = (bool *) malloc(sizeof(bool));
	if(!attr->data)
		return false;

	*(bool *)attr->data = enabled;
	return true;
}

static bool writeBool(AttributeEntryType *attr, const void *newData, int key)
{
	bool enabled;
	
	if(!newData)
		return false;
	
	enabled = *(const bool *)newData;
	
	if(!attr->data)
	{
		attr->data = (bool *) malloc(sizeof(bool));
		if(!attr->data)
			return false;
	}
	
	*(bool *)attr->data = enabled;
	
	const char *textData = boolStates[enabled ? 0 : 1];
	
	writeAttributeText(attr, textData, key);
	
	return true;
}

static bool parseInt(AttributeEntryType *attr)
{
	char *textData = attr->textData;
	int value;

	/*
	if(sscanf(textData, "%i", &value) <= 0)
	{
		attr->data = NULL;
		return false;
	}
	*/

	value = atoi(textData);

	attr->data = (int *) malloc(sizeof(int));
	if(!attr->data)
		return false;

	*(int *)attr->data = value;
	return true;
}

static bool writeInt(AttributeEntryType *attr, const void *newData, int key)
{
	const size_t maxIntStringLen = 16;
	char str[maxIntStringLen];
	int value;
	
	if(!newData)
		return false;
	
	value = *(const int *)newData;
	
	if(!attr->data)
	{
		attr->data = (int *) malloc(sizeof(int));
		if(!attr->data)
			return false;
	}
	
	*(int *)attr->data = value;
	
	/*
	if(snprintf(str, maxIntStringLen, "%i", value) <= 0)
		return false;
	*/

	itoa(value, str, 10);
	
	writeAttributeText(attr, str, key);
	
	return true;
}

void *configCopyText(int key)
{
	if(!configLoaded)
		return NULL;

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
	if(!configLoaded)
		return NULL;
	
	if(key < 0 || key >= KLast)
		return NULL;
	
	return attributes[key].data;
}

bool configDataExist(int key)
{
	if(!configLoaded)
		return false;
	
	return configGetData(key) != NULL;
}

const char *configGetKeyText(int key)
{
	if(key < 0 || key >= KLast)
		return NULL;
	
	return keyStrings[key];
}

bool configSetKeyData(int key, const void *data)
{
	AttributeEntryType *attr;
	const FunctionsEntryType *keyFunc;
	
	if(!configLoaded)
		return false;
	
	if(key < 0 || key >= KLast)
		return false;
	
	attr = &attributes[key];
	
	keyFunc = getKeyFunctions(key);
	
	if(!keyFunc || !keyFunc->write)
		panicMsg("Unimplemented key function!");
	
	if(keyFunc->write(attr, data, key))
	{
		configDirty = true;
		return true;
	}
	
	return false;
}

void configRestoreDefaults()
{
	const u32 defaultMode = BootModeNormal;

	if(!configLoaded)
		return;
	
	// truncate
	filebuf[0] = '\0';

	if(!configSetKeyData(KBootMode, &defaultMode))
		panic();
}


bool configDeleteKey(int key)
{
	AttributeEntryType *attr;
	
	if(!configLoaded)
		return false;

	if(key < 0 || key >= KLast)
		return false;
	
	attr = &attributes[key];
	
	if(removeDefinition(attr))
	{
		free(attr->data);
		attr->data = NULL;
		attr->textData = NULL;
		attr->textLength = 0;
		
		configDirty = true;
		
		return true;
	}
	
	return false;
}

static inline bool safeReadBoolKey(int key)
{
	const bool *enabled;
	
	enabled = configGetData(key);
	
	if(!enabled || *enabled == false)
		return false;
	
	return true;
}

bool configDevModeEnabled()
{
	return safeReadBoolKey(KDevMode);
}

bool configRamFirmBootEnabled()
{
	return safeReadBoolKey(KRamFirmBoot);
}
