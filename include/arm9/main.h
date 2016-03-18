
#pragma once

enum menu_state_type {
	STATE_MAIN = 0,
	STATE_FIRM_LAUNCH,
};

const char *menu_main_slots[] = {
	"Launch FIRM",
	"Options..."
};

typedef struct config_format {
	u8 dev_id;

};


extern bool unit_is_new3ds;

void devs_init();
void mount_fs();
void unit_detect();
