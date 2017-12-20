#pragma once

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

#if !__ASSEMBLER__
	#include "types.h"
#endif


// Program status register (CPSR/SPSR)
#define PSR_USER_MODE   (16)
#define PSR_FIQ_MODE    (17)
#define PSR_IRQ_MODE    (18)
#define PSR_SVC_MODE    (19)
#define PSR_ABORT_MODE  (23)
#define PSR_UNDEF_MODE  (27)
#define PSR_SYS_MODE    (31)
#define PSR_MODE_MASK   (PSR_SYS_MODE)

#define PSR_T           (1<<5)          // Thumb mode
#define PSR_F           (1<<6)          // Interrupts (FIQ) disable flag
#define PSR_I           (1<<7)          // Interrupts (IRQ) disable flag
#define PSR_A           (1<<8)          // Imprecise aborts disable flag
#define PSR_E           (1<<9)          // Big endian
#define PSR_J           (1<<24)         // Jazelle mode
#define PSR_Q           (1<<27)
#define PSR_V           (1<<28)         // Overflow flag
#define PSR_C           (1<<29)         // Carry flag
#define PSR_Z           (1<<30)         // Zero flag
#define PSR_N           (1<<31)         // Negative flag
#define PSR_INT_OFF     (PSR_I | PSR_F) // IRQ and FIQ disabled flags



#if !__ASSEMBLER__

#ifdef ARM11
#define __cpsid(flags) __asm__ volatile("cpsid " #flags : : : "memory");
#define __cpsie(flags) __asm__ volatile("cpsie " #flags : : : "memory");
#endif

static inline void __wfi(void)
{
#ifdef ARM11
	__asm__ volatile("wfi" : : : "memory");
#elif ARM9
	__asm__ volatile("mcr p15, 0, %0, c7, c0, 4" : : "r" (0) : "memory");
#endif
}

#ifdef ARM11
static inline void __wfe(void)
{
	__asm__ volatile("wfe" : : : "memory");
}

static inline void __sev(void)
{
	__asm__ volatile("sev" : : : "memory");
}
#endif

static inline u32 __getCpsr(void)
{
	u32 cpsr;
	__asm__("mrs %0, cpsr" : "=r" (cpsr) : );
	return cpsr;
}

static inline void __setCpsr_c(u32 cpsr)
{
	__asm__ volatile("msr cpsr_c, %0" : : "r" (cpsr) : "memory");
}

static inline void __setCpsr(u32 cpsr)
{
	__asm__ volatile("msr cpsr_cxsf, %0" : : "r" (cpsr) : "memory");
}

static inline u32 __getSpsr(void)
{
	u32 spsr;
	__asm__("mrs %0, spsr" : "=r" (spsr) : );
	return spsr;
}

static inline void __setSpsr_c(u32 spsr)
{
	__asm__ volatile("msr spsr_c, %0" : : "r" (spsr) : "memory");
}

static inline void __setSpsr(u32 spsr)
{
	__asm__ volatile("msr spsr_cxsf, %0" : : "r" (spsr) : "memory");
}

#ifdef ARM11
static inline u32 __getCpuId(void)
{
	u32 cpuId;
	__asm__("mrc p15, 0, %0, c0, c0, 5" : "=r" (cpuId) : );
	return cpuId & 3;
}
#endif

#endif // if !__ASSEMBLER__
