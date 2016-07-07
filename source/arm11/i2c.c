#include "types.h"
#include "IO.h"
#include "mem_map.h"
#include "util.h"



struct i2c_dev_table_entry {
	u32 bus_id;
	u8 devaddr;
};

static const struct i2c_dev_table_entry i2c_dev_table[] =
{
	{0,	0x4A},
	{0,	0x7A},
	{0,	0x78},
	{1,	0x4A},
	{1,	0x78},
	{1,	0x2C},
	{1,	0x2E},
	{1,	0x40},
	{1,	0x44}
};


// i2c mcu defines
bool i2cmcu_reg0_upper_read;
u8 i2cmcu_reg0_upper_data;
bool i2cmcu_readreg0x0_bus0_read_flag;
u8 i2cmcu_readreg0x0_bus0_first;
u8 i2cmcu_readreg0x0_bus0_second;


void i2c_wait(u32 bus_id);
void i2c_put_byte(u32 bus_id, u8 data_byte);
void i2c_start(u32 bus_id);
void i2c_set_rw(u32 bus_id, u32 read_enable);
bool i2c_test_ack(u32 bus_id);
u8 i2c_receive_byte(u32 bus_id);
void i2c_stop(u32 bus_id, u32 read_enable);
void i2c_stop_err(u32 dev_id);
bool i2c_start_put_devaddr(u32 dev_id);
bool i2c_put_devaddr_set_read(u32 dev_id);
bool i2c_put_regaddr_set_write(u32 dev_id, u32 regaddr);
u8 i2c_get_byte(u32 dev_id);
u8 i2c_get_byte_stop(u32 dev_id);
u32 i2c_readregdata(u32 dev_id, u32 regaddr, u8 *out, u32 size);
u32 i2cmcu_readregdata(u32 regaddr, u8 *outbuf, u32 size);


void i2c_wait(u32 bus_id)
{
	u8 result;
	do {
		if(bus_id == 0)
			result = REG_I2C_BUS0_CNT;
		else
			result = REG_I2C_BUS1_CNT;
	} while((result>>7) & 1);
}

void i2c_put_byte(u32 bus_id, u8 data_byte)
{
	if(bus_id == 0)
		REG_I2C_BUS0_DATA = data_byte;
	else
		REG_I2C_BUS1_DATA = data_byte;
}

void i2c_start(u32 bus_id)
{
	sleep_wait(0x100);
	if(bus_id == 0)
		REG_I2C_BUS0_CNT = 0xC2;
	else
		REG_I2C_BUS1_CNT = 0xC2;
}

void i2c_set_rw(u32 bus_id, u32 read_enable)
{
	u8 cnt = 0xC0;
	cnt |= read_enable << 5;
	cnt |= read_enable << 4;
	if(bus_id == 0)
		REG_I2C_BUS0_CNT = cnt;
	else
		REG_I2C_BUS1_CNT = cnt;
}

bool i2c_test_ack(u32 bus_id)
{
	i2c_wait(bus_id);
	sleep_wait(0x80);
	u8 result;
	if(bus_id == 0)
		result = REG_I2C_BUS0_CNT;
	else
		result = REG_I2C_BUS1_CNT;
	return ((result << 0x1B) >> 0x1F);	// 	return Ack Flag
}

u8 i2c_receive_byte(u32 bus_id)
{
	i2c_wait(bus_id);
	sleep_wait(0x100);
	if(bus_id == 0)
		return REG_I2C_BUS0_DATA;
	else
		return REG_I2C_BUS1_DATA;
}

void i2c_stop(u32 bus_id, u32 read_enable)
{
	u8 cnt = 0xC1;
	cnt |= read_enable << 5;
	if(bus_id == 0)
		REG_I2C_BUS0_CNT = cnt;
	else
		REG_I2C_BUS1_CNT = cnt;
}

void i2c_stop_err(u32 dev_id)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	if(bus_id == 0)
		REG_I2C_BUS0_CNT = 0xC5;
	else
		REG_I2C_BUS1_CNT = 0xC5;
	i2c_wait(bus_id);
	sleep_wait(0x200);
}

bool i2c_start_put_devaddr(u32 dev_id)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	u8 dev_addr = i2c_dev_table[dev_id].devaddr;
	i2c_wait(bus_id);
	i2c_put_byte(bus_id, dev_addr);
	i2c_start(bus_id);
	return i2c_test_ack(bus_id);
}

bool i2c_put_devaddr_set_read(u32 dev_id)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	u8 dev_addr = i2c_dev_table[dev_id].devaddr;
	i2c_wait(bus_id);
	dev_addr |= 1; // i2c direction: read bit set
	i2c_put_byte(bus_id, dev_addr);
	i2c_start(bus_id);
	return i2c_test_ack(bus_id);
}

bool i2c_put_regaddr_set_write(u32 dev_id, u32 regaddr)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	i2c_wait(bus_id);
	i2c_put_byte(bus_id, regaddr);
	i2c_set_rw(bus_id, 0);	// write
	return i2c_test_ack(bus_id);
}

u8 i2c_get_byte(u32 dev_id)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	i2c_wait(bus_id);
	i2c_set_rw(bus_id, 1);	// read
	return i2c_receive_byte(bus_id);
}

u8 i2c_get_byte_stop(u32 dev_id)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	i2c_wait(bus_id);
	i2c_stop(bus_id, 1);	// 1=read
	return i2c_receive_byte(bus_id);
}

bool i2c_put_data_stop(u32 dev_id, u8 data)
{
	u32 bus_id = i2c_dev_table[dev_id].bus_id;
	i2c_wait(bus_id);
	i2c_put_byte(bus_id, data);
	i2c_stop(bus_id, 0);	// 0=write
	return i2c_receive_byte(bus_id);
}

u32 i2c_readregdata(u32 dev_id, u32 regaddr, u8 *out, u32 size)
{
	int i;
	for(i=0; i<8; i++)	// try max. 8 times
	{
		if(!i2c_start_put_devaddr(dev_id) ||
			!i2c_put_regaddr_set_write(dev_id, regaddr) ||
			!i2c_put_devaddr_set_read(dev_id))
				i2c_stop_err(dev_id);
		else break;
	}
	if(i == 8)
		return 0;
	
	while(size != 1)
	{
		*out = i2c_get_byte(dev_id);
		out++;
		size--;
	}
	
	// receive the last byte
	*out = i2c_get_byte_stop(dev_id);
	
	return 1;
}

u32 i2c_writeregdata(u32 dev_id, u32 regaddr, u8 data)
{
	int i;
	for(i=0; i<8; i++)	// try max. 8 times
	{
		if(!i2c_start_put_devaddr(dev_id) ||
			!i2c_put_regaddr_set_write(dev_id, regaddr) ||
			!i2c_put_data_stop(dev_id, data))
			i2c_stop_err(dev_id);
		else break;
	}
	if(i == 8)
		return 0;
	return 1;
}

u32 i2cmcu_readregdata(u32 regaddr, u8 *outbuf, u32 size)
{
	return i2c_readregdata(3, regaddr, outbuf, size);
}

u32 i2cmcu_readreg0x0_upper(u8 *result)
{
	/*if(i2cmcu_reg0_upper_read)
	{
		result = i2cmcu_reg0_upper_data;
		return 1;
	}
	else*/
	{
		u8 data;
		u32 retval = i2cmcu_readregdata(0, &data, 1);
		if(retval != 0)
		{
			data >>= 4;
			i2cmcu_reg0_upper_data = data;
			*result = data;
		}
		return retval;
	}
}

u32 i2cmcu_readreg0x0_2bytes(u8 *byte1, u8 *byte2)
{
	u8 data[2];
	if(!i2cmcu_readregdata(0, data, 2))
		return 0;
	*byte1 = data[0] & 0xF;
	*byte2 = data[1];
	return 1;
}

u32 i2cmcu_readreg0x0_bus0()
{
	if(!i2cmcu_readreg0x0_bus0_read_flag)
	{
		i2cmcu_readreg0x0_bus0_read_flag = true;
		i2cmcu_readreg0x0_2bytes(&i2cmcu_readreg0x0_bus0_first, &i2cmcu_readreg0x0_bus0_second);
		if(i2cmcu_readreg0x0_bus0_first << 0x1C) return 1;
		if(i2cmcu_readreg0x0_bus0_second >= 9) return 1;
		i2cmcu_readreg0x0_bus0_second = 0;
		return 0;
	}
	return i2cmcu_readreg0x0_bus0_second ? 1 : 0;
}

u8 i2cmcu_readreg_hid()
{
	u8 data;
	if(!i2cmcu_readregdata(0x10, &data, 1))
		return 0;
	return data;
}

// aka i2cmcu_write_reg0x20_0x22
void i2cmcu_lcd_poweron()
{
	if(i2cmcu_readreg0x0_bus0())
		i2c_writeregdata(3, 0x22, 2);	// bit1 = lcd power enable for both screens
	else
		i2c_writeregdata(3, 0x20, 0x80);
}

// aka i2cmcu_write_reg0x20_0x22_2
void i2cmcu_lcd_backlight_poweron()
{
	if(i2cmcu_readreg0x0_bus0())
		i2c_writeregdata(3, 0x22, 0x28);	// bit3 = lower screen, bit5 = upper
	else
		i2c_writeregdata(3, 0x20, 0x20);
}

void i2cmcu_lcd_poweroff()
{
	i2c_writeregdata(3, 0x22, 1);	// bit0 = lcd power disable for both screens (also disabled backlight)
}

void i2cmcu_lcd_backlight_poweroff()
{
	i2c_writeregdata(3, 0x22, 0x14);	// bit2 = backlight disable lower, bit4 = upper
}

u32 i2c_write_regdata_dev5_dev6(bool dev5, bool dev6, u32 regaddr, u8 data)
{
	u32 result = 1;
	if(dev5)
		result &= i2c_writeregdata(5, regaddr, data);
	if(dev6)
		result &= i2c_writeregdata(6, regaddr, data);
	return result;
}

u8 i2c_write_echo_read(u32 dev_id, u32 regaddr, u8 data)
{
	int i;
	for(i=0; i<8; i++)	// try max. 8 times
	{
		if(!i2c_start_put_devaddr(dev_id) ||
			!i2c_put_regaddr_set_write(dev_id, regaddr) ||
			!i2c_put_data_stop(dev_id, data) ||
			!i2c_put_devaddr_set_read(dev_id) ||
			(i2c_get_byte(dev_id) != data))
			i2c_stop_err(dev_id);
		else break;
	}
	if(i == 8)
		return 0xFF;
	return i2c_get_byte_stop(dev_id);;
}
