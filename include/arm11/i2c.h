#pragma once

#include <stdbool.h>
#include "types.h"



// general i2c routines
u32 i2c_readregdata(u32 dev_id, u32 regaddr, u8 *out, u32 size);
u32 i2c_writeregdata(u32 dev_id, u32 regaddr, u8 data);
void i2c_wait(u32 bus_id);

// mcu
u32 i2cmcu_readregdata(u32 regaddr, u8 *outbuf, u32 size);	// this uses i2c dev 3
u32 i2cmcu_readreg0x0_upper(u8 *result);	// writes the upper 4 bits of reg 0 (dev 3) to *result (* result= data >> 4;)
u32 i2cmcu_readreg0x0_2bytes(u8 *byte1, u8 *byte2);	// reads 2 bytes from i2c mcu (dev 0) reg 0. byte1 is only the lower 4 bits of the read first byte.
void i2cmcu_lcd_poweron();
void i2cmcu_lcd_backlight_poweron();
void i2cmcu_lcd_poweroff();	// also disabled backlight
void i2cmcu_lcd_backlight_poweroff();

// mcu hid
u8 i2cmcu_readreg_hid();
#define MCU_HID_POWER_BUTTON_PRESSED		1
#define MCU_HID_POWER_BUTTON_LONG_PRESSED	1<<1
#define MCU_HID_HOME_BUTTON_PRESSED			1<<2
#define MCU_HID_HOME_BUTTON_RELEASED		1<<3
#define MCU_HID_SHELL_GOT_CLOSED			1<<5
#define MCU_HID_SHELL_GOT_OPENED			1<<6

// device 5/6
u32 i2c_write_regdata_dev5_dev6(bool dev5, bool dev6, u32 regaddr, u8 data);
u8 i2c_write_echo_read(u32 dev_id, u32 regaddr, u8 data);	// only used for dev5 and dev6

