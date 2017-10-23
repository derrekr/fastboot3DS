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

#include "types.h"
#include "mem_map.h"
#include "arm11/hardware/interrupt.h"
#include "arm11/start.h"


#define REG_CFG11_FIQ_CNT    *((vu8* )(IO_MEM_ARM9_ARM11 + 0x40000 + 0x0104))

#define CPU_II_REGS_BASE     (MPCORE_PRIV_REG_BASE + 0x100)
#define REG_CPU_II_CNT       *((vu32*)(CPU_II_REGS_BASE + 0x00))
#define REG_CPU_II_MASK      *((vu32*)(CPU_II_REGS_BASE + 0x04))
#define REG_CPU_II_BIN_POI   *((vu32*)(CPU_II_REGS_BASE + 0x08))
#define REG_CPU_II_AKN       *((vu32*)(CPU_II_REGS_BASE + 0x0C))
#define REG_CPU_II_EOI       *((vu32*)(CPU_II_REGS_BASE + 0x10))
#define REG_CPU_II_RUN_PRIO  *((vu32*)(CPU_II_REGS_BASE + 0x14))
#define REG_CPU_II_HIGH_PEN  *((vu32*)(CPU_II_REGS_BASE + 0x18))

#define GID_REGS_BASE        (MPCORE_PRIV_REG_BASE + 0x1000)
#define REG_GID_CNT          *((vu32*)(GID_REGS_BASE + 0x000))
#define REG_GID_CONTR_TYPE   *((vu32*)(GID_REGS_BASE + 0x004))
#define REGs_GID_ENA_SET      ((vu32*)(GID_REGS_BASE + 0x100))
#define REGs_GID_ENA_CLR      ((vu32*)(GID_REGS_BASE + 0x180))
#define REGs_GID_PEN_SET      ((vu32*)(GID_REGS_BASE + 0x200))
#define REGs_GID_PEN_CLR      ((vu32*)(GID_REGS_BASE + 0x280))
#define REGs_GID_ACTIVE_BIT   ((vu32*)(GID_REGS_BASE + 0x300))
#define REGs_GID_IPRIO        ((vu32*)(GID_REGS_BASE + 0x400))
#define REGs_GID_ITARG        ((vu32*)(GID_REGS_BASE + 0x800))
#define REGs_GID_ICONF        ((vu32*)(GID_REGS_BASE + 0xC00))
#define REGs_GID_LINE_LEV     ((vu32*)(GID_REGS_BASE + 0xD00))
#define REG_GID_SW_INT       *((vu32*)(GID_REGS_BASE + 0xF00))
#define REG_GID_PERI_INFO0   *((vu32*)(GID_REGS_BASE + 0xFE0))
#define REG_GID_PERI_INFO1   *((vu32*)(GID_REGS_BASE + 0xFE4))
#define REG_GID_PERI_INFO2   *((vu32*)(GID_REGS_BASE + 0xFE8))
#define REG_GID_PERI_INFO3   *((vu32*)(GID_REGS_BASE + 0xFEC))
#define REG_GID_PRIME_CELL0  *((vu32*)(GID_REGS_BASE + 0xFF0))
#define REG_GID_PRIME_CELL1  *((vu32*)(GID_REGS_BASE + 0xFF4))
#define REG_GID_PRIME_CELL2  *((vu32*)(GID_REGS_BASE + 0xFF8))
#define REG_GID_PRIME_CELL3  *((vu32*)(GID_REGS_BASE + 0xFFC))


IrqHandler privIrqHandlerTable[4][32] = {0}; // Table for private MPCore interrupts
IrqHandler irqHandlerTable[96] = {0};        // There are 96 external interrupts (total 128)



void IRQ_init(void)
{
	// Disable the interrupt interface for this CPU
	REG_CPU_II_CNT = 0;


	if(!getCpuId())
	{
		// Disable FIQs
		REG_CFG11_FIQ_CNT = 1;


		// Disable the global interrupt distributor
		REG_GID_CNT = 0;

		// Disable all 128 interrupts
		REGs_GID_ENA_CLR[0] = 0xFFFFFFFFu; // Interrupts 0-15 cant be disabled
		REGs_GID_ENA_CLR[1] = 0xFFFFFFFFu;
		REGs_GID_ENA_CLR[2] = 0xFFFFFFFFu;
		REGs_GID_ENA_CLR[3] = 0xFFFFFFFFu;

		// Set all pending interrupts to inactive state
		REGs_GID_PEN_CLR[0] = 0xFFFFFFFFu; // Interrupt 0-15 can't be set to inactive apparently
		REGs_GID_PEN_CLR[1] = 0xFFFFFFFFu;
		REGs_GID_PEN_CLR[2] = 0xFFFFFFFFu;
		REGs_GID_PEN_CLR[3] = 0xFFFFFFFFu;

		// Set all 128 interrupts to lowest priority
		for(u32 i = 0; i < 32; i++) REGs_GID_IPRIO[i] = 0xE0E0E0E0u;

		// Set all 128 interrupts to target no CPU.
		// Interrupt 0-31 can't be changed
		for(u32 i = 8; i < 32; i++) REGs_GID_ITARG[i] = 0;

		// Set all interrupts to rising edge sensitive and 1-N software model
		for(u32 i = 0; i < 8; i++) REGs_GID_ICONF[i] = 0xFFFFFFFFu;

		// Enable the global interrupt distributor
		REG_GID_CNT = 1;
	}
	else
	{
		REGs_GID_PEN_CLR[0] = 0xFFFFFFFFu;

		for(u32 i = 0; i < 8; i++) REGs_GID_IPRIO[i] = 0xE0E0E0E0u;

		REGs_GID_ICONF[0] = 0xFFFFFFFFu;
		REGs_GID_ICONF[1] = 0xFFFFFFFFu;
	}


	// Mask no interrupt
	REG_CPU_II_MASK = 0xF0;
	// All priority bits are compared for pre-emption
	REG_CPU_II_BIN_POI = 3;
	// Enable the interrupt interface for this CPU
	REG_CPU_II_CNT = 1;

	// Get rid of all interrupts stuck in pending/active state
	u32 tmp;
	do
	{
		tmp = REG_CPU_II_AKN; // Aknowledge
		REG_CPU_II_EOI = tmp; // End of interrupt
	} while(tmp != 1023);


	leaveCriticalSection(); // Enables interrupts
}

void IRQ_registerHandler(Interrupt id, u8 prio, u8 cpuMask, bool edgeTriggered, IrqHandler handler)
{
	enterCriticalSection();

	const u32 cpuId = getCpuId();

	if(!cpuMask) cpuMask = 1u<<cpuId;

	if(handler)
	{
		if(id < 32) privIrqHandlerTable[cpuId][id] = handler;
		else irqHandlerTable[id - 32] = handler;
	}

	u32 shift = (id % 4 * 8) + 4;
	u32 tmp = REGs_GID_IPRIO[id>>2] & ~(0xFu<<shift);
	REGs_GID_IPRIO[id>>2] = tmp | (u32)prio<<shift;

	shift = id % 4 * 8;
	tmp = REGs_GID_ITARG[id>>2] & ~(0xFu<<shift);
	REGs_GID_ITARG[id>>2] = tmp | (u32)cpuMask<<shift;

	shift = (id % 16 * 2) + 1;
	tmp = REGs_GID_ICONF[id>>4] & ~(1u<<shift);
	REGs_GID_ICONF[id>>4] = tmp | (u32)edgeTriggered<<shift;

	REGs_GID_ENA_SET[id>>5] = 1u<<(id % 32);

	leaveCriticalSection();
}

void IRQ_unregisterHandler(Interrupt id)
{
	enterCriticalSection();

	REGs_GID_ENA_CLR[id>>5] = 1u<<(id % 32);

	if(id < 32) privIrqHandlerTable[getCpuId()][id] = (IrqHandler)NULL;
	else irqHandlerTable[id - 32] = (IrqHandler)NULL;

	leaveCriticalSection();
}

void IRQ_setPriority(Interrupt id, u8 prio)
{
	enterCriticalSection();

	u32 shift = (id % 4 * 8) + 4;
	u32 tmp = REGs_GID_IPRIO[id>>2] & ~(0xFu<<shift);
	REGs_GID_IPRIO[id>>2] = tmp | (u32)prio<<shift;

	leaveCriticalSection();
}

void IRQ_softwareInterrupt(Interrupt id, u8 cpuMask)
{
	REG_GID_SW_INT = (u32)cpuMask<<16 | id;
}
