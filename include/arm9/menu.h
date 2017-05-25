#pragma once

#include "arm9/ui.h"
#include "arm9/menu_firmloader.h"
#include "arm9/menu_nand.h"
#include "arm9/menu_options.h"
#include "arm9/menu_filebrowse.h"
#include "arm9/menu_update.h"
#include "arm9/menu_credits.h"

enum menu_state_type {
	MENU_STATE_MAIN = 0,
	MENU_STATE_NAND_MENU,
	MENU_STATE_OPTIONS_MENU,
	MENU_STATE_OPTIONS_MODE,
	MENU_STATE_FIRM_LAUNCH_SETTINGS,
	MENU_STATE_NAND_BACKUP,
	MENU_STATE_NAND_RESTORE,
	MENU_STATE_NAND_FLASH_FIRM,
	MENU_STATE_FIRM_LAUNCH,
	MENU_STATE_BROWSER,
	MENU_STATE_UPDATE,
	MENU_STATE_CREDITS,
	MENU_STATE_EXIT,

	STATE_PREVIOUS	// pseudo state for menuSetReturnToState()
};

typedef struct {
	const char *name;
	const enum menu_state_type state;
} named_state;

typedef struct {
	const u8 count;	// holds the number of available options
	const named_state options[];
} menu_state_options;

enum menu_events {
	MENU_EVENT_NONE = 0,
	MENU_EVENT_HOME_PRESSED,
	MENU_EVENT_POWER_PRESSED,
	MENU_EVENT_SD_CARD_INSERTED,
	MENU_EVENT_SD_CARD_REMOVED,
	MENU_EVENT_STATE_CHANGE
};

extern enum menu_state_type menu_state;
extern enum menu_state_type menu_next_state;

extern enum menu_state_type menu_previous_states[8];
extern int menu_previous_states_count;

extern int menu_event_state;

// Enable or disable the pseudo VBlank in the menu.
void menuSetVBlank(bool enable);

int enter_menu(int initial_state);

// Use this to change to another sub-menu.
// Call menuUpdateGlobalState and menuActState afterwards.
void menuSetReturnToState(int state);
// Use this to change back to a previous sub-menu.
// Call menuUpdateGlobalState and menuActState afterwards.
void menuSetEnterNextState(int state);
// This must be called once in each (sub-) menu iteration.
// Returns an event code.
int menuUpdateGlobalState(void);
// This must be called once in each (sub-) menu iteration,
// after menuUpdateGlobalState has been called.
void menuActState(void);
// Shows a windowed text, updates menu state.
void menuPrintPrompt(const char *text);
// Waits for any key being pressed, updates menu state.
void menuWaitForAnyPadkey();

