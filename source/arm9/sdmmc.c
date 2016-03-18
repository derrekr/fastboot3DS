/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014, Normmatt
 * Copyright (c) 2016, derrek
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */
 

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "types.h"
#include "util.h"
#include "arm9/dev.h"
#include "arm9/sdmmc.h"

#define DATA32_SUPPORT


// ------------------------------------------ sdmmc driver ------------------------------------------

struct mmcdevice handleNAND;
struct mmcdevice handleSD;

mmcdevice *getMMCDevice(int drive)
{
	if(drive==0) return &handleNAND;
	return &handleSD;
}

int geterror(struct mmcdevice *ctx)
{
	return (ctx->error << 29) >> 31;
}

void inittarget(struct mmcdevice *ctx)
{
	sdmmc_mask16(REG_SDPORTSEL,0x3,(uint16_t)ctx->devicenumber);
	setckl(ctx->clk);
	if(ctx->SDOPT == 0)
	{
		sdmmc_mask16(REG_SDOPT,0,0x8000);
	}
	else
	{
		sdmmc_mask16(REG_SDOPT,0x8000,0);
	}
	
}

void sdmmc_send_command(struct mmcdevice *ctx, uint32_t cmd, uint32_t args)
{
	bool getSDRESP = (cmd << 15) >> 31;
	uint16_t flags = (cmd << 15) >> 31;
	const bool readdata = cmd & 0x20000;
	const bool writedata = cmd & 0x40000;
	
	if(readdata || writedata)
	{
		flags |= TMIO_STAT0_DATAEND;
	}
	
	ctx->error = 0;
	
	while((sdmmc_read16(REG_SDSTATUS1) & TMIO_STAT1_CMD_BUSY)); //mmc working?
	sdmmc_write16(REG_SDIRMASK0,0);
	sdmmc_write16(REG_SDIRMASK1,0);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);
	sdmmc_mask16(REG_DATACTL32,0x1800,0);
	sdmmc_write16(REG_SDCMDARG0,args &0xFFFF);
	sdmmc_write16(REG_SDCMDARG1,args >> 16);
	sdmmc_write16(REG_SDCMD,cmd &0xFFFF);
	
	uint32_t size = ctx->size;
	uint16_t *dataPtr = (uint16_t*)ctx->data;
	uint32_t *dataPtr32 = (uint32_t*)ctx->data;
	
	bool useBuf = ( NULL != dataPtr );
	bool useBuf32 = (useBuf && (0 == (3 & ((uint32_t)dataPtr))));
	
	uint16_t status0 = 0;
	while(1)
	{
		vu16 status1 = sdmmc_read16(REG_SDSTATUS1);
#ifdef DATA32_SUPPORT
		vu16 ctl32 = sdmmc_read16(REG_DATACTL32);
		if((ctl32 & 0x100))
#else
		if((status1 & TMIO_STAT1_RXRDY))
#endif
		{
			if(readdata)
			{
				if(useBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_RXRDY, 0);
					if(size > 0x1FF)
					{
						#ifdef DATA32_SUPPORT
						if(useBuf32)
						{
							for(int i = 0; i<0x200; i+=4)
							{
								*dataPtr32++ = sdmmc_read32(REG_SDFIFO32);
							}
						}
						else 
						{
						#endif
							for(int i = 0; i<0x200; i+=2)
							{
								*dataPtr++ = sdmmc_read16(REG_SDFIFO);
							}
						#ifdef DATA32_SUPPORT
						}
						#endif
						size -= 0x200;
					}
				}
				
				sdmmc_mask16(REG_DATACTL32, 0x800, 0);
			}
		}
#ifdef DATA32_SUPPORT
		if(!(ctl32 & 0x200))
#else
		if((status1 & TMIO_STAT1_TXRQ))
#endif
		{
			if(writedata)
			{
				if(useBuf)
				{
					sdmmc_mask16(REG_SDSTATUS1, TMIO_STAT1_TXRQ, 0);
					if(size > 0x1FF)
					{
						#ifdef DATA32_SUPPORT
						for(int i = 0; i<0x200; i+=4)
						{
							sdmmc_write32(REG_SDFIFO32,*dataPtr32++);
						}
						#else
						for(int i = 0; i<0x200; i+=2)
						{
							sdmmc_write16(REG_SDFIFO,*dataPtr++);
						}
						#endif
						size -= 0x200;
					}
				}
				
				sdmmc_mask16(REG_DATACTL32, 0x1000, 0);
			}
		}
		if(status1 & TMIO_MASK_GW)
		{
			ctx->error |= 4;
			break;
		}
		
		if(!(status1 & TMIO_STAT1_CMD_BUSY))
		{
			status0 = sdmmc_read16(REG_SDSTATUS0);
			if(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_CMDRESPEND)
			{
				ctx->error |= 0x1;
			}
			if(status0 & TMIO_STAT0_DATAEND)
			{
				ctx->error |= 0x2;
			}
			
			if((status0 & flags) == flags)
				break;
		}
	}
	ctx->stat0 = sdmmc_read16(REG_SDSTATUS0);
	ctx->stat1 = sdmmc_read16(REG_SDSTATUS1);
	sdmmc_write16(REG_SDSTATUS0,0);
	sdmmc_write16(REG_SDSTATUS1,0);
	
	if(getSDRESP != 0)
	{
		ctx->ret[0] = sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1) << 16);
		ctx->ret[1] = sdmmc_read16(REG_SDRESP2) | (sdmmc_read16(REG_SDRESP3) << 16);
		ctx->ret[2] = sdmmc_read16(REG_SDRESP4) | (sdmmc_read16(REG_SDRESP5) << 16);
		ctx->ret[3] = sdmmc_read16(REG_SDRESP6) | (sdmmc_read16(REG_SDRESP7) << 16);
	}
}

int sdmmc_sdcard_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *in)
{
	if(handleSD.isSDHC == 0) sector_no <<= 9;
	inittarget(&handleSD);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleSD.data = in;
	handleSD.size = numsectors << 9;
	sdmmc_send_command(&handleSD,0x52C19,sector_no);
	return geterror(&handleSD);
}

int sdmmc_sdcard_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out)
{
	if(handleSD.isSDHC == 0) sector_no <<= 9;
	inittarget(&handleSD);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleSD.data = out;
	handleSD.size = numsectors << 9;
	sdmmc_send_command(&handleSD,0x33C12,sector_no);
	return geterror(&handleSD);
}

int sdmmc_nand_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out)
{
	if(handleNAND.isSDHC == 0) sector_no <<= 9;
	inittarget(&handleNAND);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleNAND.data = out;
	handleNAND.size = numsectors << 9;
	sdmmc_send_command(&handleNAND,0x33C12,sector_no);
	inittarget(&handleSD);
	return geterror(&handleNAND);
}

int sdmmc_nand_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *in) //experimental
{
	if(handleNAND.isSDHC == 0) sector_no <<= 9;
	inittarget(&handleNAND);
	sdmmc_write16(REG_SDSTOP,0x100);
#ifdef DATA32_SUPPORT
	sdmmc_write16(REG_SDBLKCOUNT32,numsectors);
	sdmmc_write16(REG_SDBLKLEN32,0x200);
#endif
	sdmmc_write16(REG_SDBLKCOUNT,numsectors);
	handleNAND.data = in;
	handleNAND.size = numsectors << 9;
	sdmmc_send_command(&handleNAND,0x52C19,sector_no);
	inittarget(&handleSD);
	return geterror(&handleNAND);
}

static uint32_t calcSDSize(uint8_t* csd, int type)
{
  uint32_t result=0;
  if(type == -1) type = csd[14] >> 6;
  switch(type)
  {
    case 0:
      {
        uint32_t block_len=csd[9]&0xf;
        block_len=1<<block_len;
        uint32_t mult=(csd[4]>>7)|((csd[5]&3)<<1);
        mult=1<<(mult+2);
        result=csd[8]&3;
        result=(result<<8)|csd[7];
        result=(result<<2)|(csd[6]>>6);
        result=(result+1)*mult*block_len/512;
      }
      break;
    case 1:
      result=csd[7]&0x3f;
      result=(result<<8)|csd[6];
      result=(result<<8)|csd[5];
      result=(result+1)*1024;
      break;
  }
  return result;
}

void sdmmc_init()
{
	//NAND
	handleNAND.isSDHC = 0;
	handleNAND.SDOPT = 0;
	handleNAND.res = 0;
	handleNAND.initarg = 1;
	handleNAND.clk = 0x80;
	handleNAND.devicenumber = 1;
	
	//SD
	handleSD.isSDHC = 0;
	handleSD.SDOPT = 0;
	handleSD.res = 0;
	handleSD.initarg = 0;
	handleSD.clk = 0x80;
	handleSD.devicenumber = 0;
	
	*(vu16*)0x10006100 &= 0xF7FFu; //SDDATACTL32
	*(vu16*)0x10006100 &= 0xEFFFu; //SDDATACTL32
#ifdef DATA32_SUPPORT
	*(vu16*)0x10006100 |= 0x402u; //SDDATACTL32
#else
	*(vu16*)0x10006100 |= 0x402u; //SDDATACTL32
#endif
	*(vu16*)0x100060D8 = (*(vu16*)0x100060D8 & 0xFFDD) | 2;
#ifdef DATA32_SUPPORT
	*(vu16*)0x10006100 &= 0xFFFFu; //SDDATACTL32
	*(vu16*)0x100060D8 &= 0xFFDFu; //SDDATACTL
	*(vu16*)0x10006104 = 512; //SDBLKLEN32
#else
	*(vu16*)0x10006100 &= 0xFFFDu; //SDDATACTL32
	*(vu16*)0x100060D8 &= 0xFFDDu; //SDDATACTL
	*(vu16*)0x10006104 = 0; //SDBLKLEN32
#endif
	*(vu16*)0x10006108 = 1; //SDBLKCOUNT32
	*(vu16*)0x100060E0 &= 0xFFFEu; //SDRESET
	*(vu16*)0x100060E0 |= 1u; //SDRESET
	*(vu16*)0x10006020 |= TMIO_MASK_ALL; //SDIR_MASK0
	*(vu16*)0x10006022 |= TMIO_MASK_ALL>>16; //SDIR_MASK1
	*(vu16*)0x100060FC |= 0xDBu; //SDCTL_RESERVED7
	*(vu16*)0x100060FE |= 0xDBu; //SDCTL_RESERVED8
	*(vu16*)0x10006002 &= 0xFFFCu; //SDPORTSEL
#ifdef DATA32_SUPPORT
	*(vu16*)0x10006024 = 0x20;
	*(vu16*)0x10006028 = 0x40EE;
#else
	*(vu16*)0x10006024 = 0x40; //Nintendo sets this to 0x20
	*(vu16*)0x10006028 = 0x40EB; //Nintendo sets this to 0x40EE
#endif
	*(vu16*)0x10006002 &= 0xFFFCu; ////SDPORTSEL
	*(vu16*)0x10006026 = 512; //SDBLKLEN
	*(vu16*)0x10006008 = 0; //SDSTOP
	
	inittarget(&handleSD);
}

int Nand_Init()
{
	inittarget(&handleNAND);
	sleep_wait(0xF000);
	sdmmc_send_command(&handleNAND,0,0);
	
	for(int i=0; i<100; i++)
	{
		for(int j=0; j<20; j++)
		{
			sdmmc_send_command(&handleNAND,0x10701,0x100000);
			if(handleNAND.error & 1) break;
		}
		if((handleNAND.ret[0] & 0x80000000) != 0) break;
	}
	
	sdmmc_send_command(&handleNAND,0x10602,0x0);
	if((handleNAND.error & 0x4))return -1;
	
	sdmmc_send_command(&handleNAND,0x10403,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;
	
	sdmmc_send_command(&handleNAND,0x10609,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;
	
	handleNAND.total_size = calcSDSize((uint8_t*)&handleNAND.ret[0],0);
	handleNAND.clk = 1;
	setckl(1);
	
	sdmmc_send_command(&handleNAND,0x10407,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;
	
	handleNAND.SDOPT = 1;
	
	sdmmc_send_command(&handleNAND,0x10506,0x3B70100);
	if((handleNAND.error & 0x4))return -1;
	
	sdmmc_send_command(&handleNAND,0x10506,0x3B90100);
	if((handleNAND.error & 0x4))return -1;
	
	sdmmc_send_command(&handleNAND,0x1040D,handleNAND.initarg << 0x10);
	if((handleNAND.error & 0x4))return -1;
	
	sdmmc_send_command(&handleNAND,0x10410,0x200);
	if((handleNAND.error & 0x4))return -1;
	
	handleNAND.clk |= 0x200; 
	
	inittarget(&handleSD);
	
	return 0;
}

int SD_Init()
{
	inittarget(&handleSD);
	sleep_wait(0xF000);
	
	if(!(sdmmc_read16(REG_SDSTATUS0) & TMIO_STAT0_SIGSTATE)) return -1;
	
	sdmmc_send_command(&handleSD,0,0);
	sdmmc_send_command(&handleSD,0x10408,0x1AA);
	uint32_t temp = (handleSD.error & 0x1) << 0x1E;
	
	//int count = 0;
	uint32_t temp2 = 0;	
	
	for(int i=0; i<100; i++)
	{
		for(int j=0; j<20; j++)
		{
			sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
			sdmmc_send_command(&handleSD,0x10769,0x00FF8000 | temp);
			temp2 = 1;
			if(handleSD.error & 1) break;
		}
		if((handleSD.ret[0] & 0x80000000) != 0) break;
	}
	

	if(!((handleSD.ret[0] >> 30) & 1) || !temp)
		temp2 = 0;
	
	handleSD.isSDHC = temp2;
	
	sdmmc_send_command(&handleSD,0x10602,0);
	if((handleSD.error & 0x4)) return -1;
	
	sdmmc_send_command(&handleSD,0x10403,0);
	if((handleSD.error & 0x4)) return -1;
	handleSD.initarg = handleSD.ret[0] >> 0x10;
	
	sdmmc_send_command(&handleSD,0x10609,handleSD.initarg << 0x10);
	if((handleSD.error & 0x4)) return -1;
	
	handleSD.total_size = calcSDSize((uint8_t*)&handleSD.ret[0],-1);
	handleSD.clk = 1;
	setckl(1);
	
	sdmmc_send_command(&handleSD,0x10507,handleSD.initarg << 0x10);
	if((handleSD.error & 0x4)) return -1;
	
	sdmmc_send_command(&handleSD,0x10437,handleSD.initarg << 0x10);
	if((handleSD.error & 0x4)) return -1;
	
	handleSD.SDOPT = 1;
	sdmmc_send_command(&handleSD,0x10446,0x2);
	if((handleSD.error & 0x4)) return -1;
	
	sdmmc_send_command(&handleSD,0x1040D,handleSD.initarg << 0x10);
	if((handleSD.error & 0x4)) return -1;
	
	sdmmc_send_command(&handleSD,0x10410,0x200);
	if((handleSD.error & 0x4)) return -1;
	handleSD.clk |= 0x200;
	
	return 0;
}
