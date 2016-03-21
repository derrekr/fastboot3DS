#pragma once

enum menu_state_type {
	STATE_MAIN = 0,
	STATE_NAND_MENU,
	STATE_NAND_BACKUP,
	STATE_NAND_RESTORE,
	STATE_OPTIONS_MENU,
	STATE_FIRM_LAUNCH,
};

typedef struct {
	const char *name;
	const enum menu_state_type state;
} named_state;

typedef struct {
	const u8 count;	// holds the number of available options
	const named_state options[];
} menu_state_options;

const menu_state_options menu_main = {
	3,
	{
		{"Launch FIRM", STATE_FIRM_LAUNCH},
		{"NAND tools...", STATE_NAND_MENU},
		{"Options...", STATE_OPTIONS_MENU}
	}
};

const menu_state_options menu_nand = {	
	2,
	{
		{"Backup NAND", STATE_NAND_BACKUP},
		{"Restore NAND", STATE_NAND_RESTORE}
	}
};

// state -> menu_state_options instance
const menu_state_options *options_lookup[] = {
	&menu_main, // STATE_MAIN
	&menu_nand // STATE_NAND_MENU
};

// Shut up gcc warning
/*typedef struct config_format {
	u8 dev_id;

};*/


extern bool unit_is_new3ds;

void devs_init();
void mount_fs();
void unit_detect();
