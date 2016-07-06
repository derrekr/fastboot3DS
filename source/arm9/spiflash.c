#include "types.h"
#include "IO.h"
#include "arm9/spiflash.h"



void spi_busy_wait()
{
    while(SPI_REGS_BUS2_CNT & 0x80);
}

void spi_put_byte(u8 data)
{
    SPI_REGS_BUS2_DATA = data;
    spi_busy_wait();
}

u8 spi_receive_byte()
{
    // clock out a dummy byte
    SPI_REGS_BUS2_DATA = 0x00;
    spi_busy_wait();
    return SPI_REGS_BUS2_DATA;
}

// select spiflash if select=true, deselect otherwise 
void spiflash_select(bool select)
{
    // select device 1, enable SPI bus
    SPI_REGS_BUS2_CNT = 0x8100 | (select << 11);
}

bool spiflash_get_status()
{
    u8 resp;

    spi_busy_wait();
    spiflash_select(1);
    spi_put_byte(SPIFLASH_CMD_RDSR);
    spiflash_select(0);
    resp = spi_receive_byte();

    if(resp & 1) return false;
    return true;
}

void spiflash_read(u32 offset, u32 size, u8 *buf)
{
	spi_busy_wait();
	spiflash_select(1);
	spi_put_byte(SPIFLASH_CMD_READ);
	
	// write addr (24-bit, msb first)	
	for(int i=0; i<3; i++)
	{
		offset <<= 8;
		spi_put_byte((offset >> 24) & 0xFF);
	}
	
	// read bytes
	for(u32 i=0; i<size; i++)
		buf[i] = spi_receive_byte();
	
	// end of read
	spiflash_select(0);
	spi_receive_byte();
}
