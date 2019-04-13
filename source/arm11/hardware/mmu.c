/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2018 derrek, profi200
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
#include "fb_assert.h"
#include "arm11/hardware/scu.h"
#include "mmio.h"
#include "arm.h"


// Mem permissions
#define PERM_NA                              (0b000000u)
#define PERM_PRIV_RW_USR_NA                  (0b000001u)
#define PERM_PRIV_RW_USR_RO                  (0b000010u)
#define PERM_PRIV_RW_USR_RW                  (0b000011u)
#define PERM_PRIV_RO_USR_NA                  (0b100001u) // Doesn't work for supersections
#define PERM_PRIV_RO_USR_RO                  (0b100010u) // Doesn't work for supersections

// Predefined mem attributes. Only bits 0, 1 and 10-12 matter
#define ATTR_STRONGLY_ORDERED                (0b0000000000000u) // Always shared
#define ATTR_SHARED_DEVICE                   (0b0000000000001u) // Always shared
#define ATTR_NORM_WRITE_TROUGH_NO_ALLOC      (0b0000000000010u)
#define ATTR_NORM_WRITE_BACK_NO_ALLOC        (0b0000000000011u)
#define ATTR_NORM_NONCACHABLE                (0b0010000000000u)
#define ATTR_NORM_WRITE_BACK_ALLOC           (0b0010000000011u)
#define ATTR_NONSHARED_DEVICE                (0b0100000000000u) // Always non-shared

// Policies for custom normal memory attributes
#define POLI_NONCACHABLE_UNBUFFERED          (0b00u)
#define POLI_WRITE_BACK_ALLOC_BUFFERED       (0b01u)
#define POLI_WRITE_THROUGH_NO_ALLOC_BUFFERED (0b10u) // Behaves as noncacheable on ARM11 MPCore
#define POLI_WRITE_BACK_NO_ALLOC_BUFFERED    (0b11u)

// Make custom normal memory attributes
#define CUSTOM_ATTR(outer, inner)            (1u<<12 | (outer)<<10 | (inner))

// Converts the attribute bits from L1 format to L2 format.
// Required for mmuMapPages().
#define L1_TO_L2(attr)                       (((attr)>>6 | (attr)) & 0x73)



/**
 * @brief      Maps up to 256 16 MiB sections of memory. Domain is always 0.
 *
 * @param[in]  va      The virtual address base. Must be aligned to 16 MiB.
 * @param[in]  pa      The physical address base. Must be aligned to 16 MiB.
 * @param[in]  num     The number of sections to map.
 * @param[in]  access  The access permission bits.
 * @param[in]  xn      If this memory should be marked as execute never.
 * @param[in]  attr    Other attribute bits like caching.
 */
static void mmuMapSupersections(u32 va, u32 pa, u32 num, u8 access, bool xn, u32 attr)
{
	fb_assert(!(va & ~0xFF000000));
	fb_assert(!(pa & ~0xFF000000));
	fb_assert(num < 256);

	for(u32 i = 0; i < 0x1000000 * num; i += 0x1000000)
	{
		const u32 l1Ss = (va + i)>>20;
		for(u32 n = 0; n < 16; n++)
		{
			((u32*)A11_MMU_TABLES_BASE)[l1Ss + n] = (pa + i) | 1u<<18 | access<<10 |
			                                        xn<<4 | attr<<2 | 0b10u;
		}
	}
}

/**
 * @brief      Maps up to 4096 1 MiB sections of memory.
 *
 * @param[in]  va      The virtual address base. Must be aligned to 1 MiB.
 * @param[in]  pa      The physical address base. Must be aligned to 1 MiB.
 * @param[in]  num     The number of sections to map.
 * @param[in]  shared  If the sections are shared memory.
 * @param[in]  access  The access permission bits.
 * @param[in]  domain  One of the 16 possible domains.
 * @param[in]  xn      If this memory should be marked as execute never.
 * @param[in]  attr    Other attribute bits like caching.
 */
static void mmuMapSections(u32 va, u32 pa, u32 num, bool shared, u8 access, u8 domain, bool xn, u32 attr)
{
	fb_assert(!(va & ~0xFFF00000));
	fb_assert(!(pa & ~0xFFF00000));
	fb_assert(num < 4096);

	for(u32 i = 0; i < 0x100000 * num; i += 0x100000)
	{
		((u32*)A11_MMU_TABLES_BASE)[(va + i)>>20] = (pa + i) | shared<<16 | access<<10 |
		                                            domain<<5 | xn<<4 | attr<<2 | 0b10u;
	}
}

/**
 * @brief      Maps up to 256 4 KiB pages of memory.
 * @brief      The mapped range must not cross the next section.
 *
 * @param[in]  va       The virtual address base. Must be aligned to 4 KiB.
 * @param[in]  pa       The physical address base. Must be aligned to 4 KiB.
 * @param[in]  num      The number of pages to map. Must be <= 128.
 * @param      l2Table  The L2 MMU table address base for this mapping.
 * @param[in]  shared   If the pages are shared memory.
 * @param[in]  access   The access permission bits.
 * @param[in]  domain   One of the 16 possible domains.
 * @param[in]  xn       If this memory should be marked as execute never.
 * @param[in]  attr     Other attribute bits like caching.
 */
static void mmuMapPages(u32 va, u32 pa, u32 num, u32 *l2Table, bool shared, u8 access, u8 domain, bool xn, u32 attr)
{
	fb_assert(!(va & ~0xFFFFF000));
	fb_assert(!(pa & ~0xFFFFF000));
	fb_assert(num < 256);
	fb_assert(!((u32)l2Table & ~0xFFFFFC00));

	((u32*)A11_MMU_TABLES_BASE)[va>>20] = (u32)l2Table | domain<<5 | 0b01u;

	for(u32 i = 0; i < 0x1000 * num; i += 0x1000)
	{
		l2Table[(va + i)>>12 & 0xFF] = ((pa + i) & 0xFFFFF000) | shared<<10 | access<<4 | attr<<2 | 0b10u | xn;
	}
}

void setupMmu(void)
{
	// FCSE PID Register (FCSE PID = 0)
	// Note: This must be 0 before disabling the MMU otherwise UB
	__setFcsepidr(0);
	// Context ID Register (ASID = 0, PROCID = 0)
	__setCidr(0);
	// TTBR0 address shared page table walk and outer cachable write-through, no allocate on write
	__setTtbr0(A11_MMU_TABLES_BASE | 0x12);
	// Use the 16 KB L1 table only
	__setTtbcr(0);
	// Domain 0 = client, remaining domains all = no access
	__setDacr(1);


	static volatile bool syncFlag = false;
	if(!__getCpuId())
	{
		// Clear L1 and L2 tables
		iomemset((u32*)A11_MMU_TABLES_BASE, 0, 0x4C00);

		// IO mem mapping
		mmuMapSections(IO_MEM_ARM9_ARM11, IO_MEM_ARM9_ARM11, 4, true,
		               PERM_PRIV_RW_USR_NA, 0, true, ATTR_SHARED_DEVICE);

		// MPCore private region mapping
		mmuMapPages(MPCORE_PRIV_REG_BASE, MPCORE_PRIV_REG_BASE, 2,
		            (u32*)(A11_MMU_TABLES_BASE + 0x4000), false, PERM_PRIV_RW_USR_NA,
		            0, true, L1_TO_L2(ATTR_NONSHARED_DEVICE));

		// VRAM mapping
		mmuMapSections(VRAM_BASE, VRAM_BASE, 6, true, PERM_PRIV_RW_USR_NA, 0, true, ATTR_NORM_WRITE_TROUGH_NO_ALLOC);

		// AXIWRAM core 0/1 stack mapping
		mmuMapPages(A11_C0_STACK_START, A11_C0_STACK_START, 4, (u32*)(A11_MMU_TABLES_BASE + 0x4400),
		            true, PERM_PRIV_RW_USR_NA, 0, true,
		            L1_TO_L2(CUSTOM_ATTR(POLI_WRITE_BACK_ALLOC_BUFFERED, POLI_WRITE_BACK_ALLOC_BUFFERED)));

		// AXIWRAM MMU table mapping
		mmuMapPages(A11_MMU_TABLES_BASE, A11_MMU_TABLES_BASE, 5, (u32*)(A11_MMU_TABLES_BASE + 0x4400), true,
		            PERM_PRIV_RO_USR_NA, 0, true, L1_TO_L2(ATTR_NORM_NONCACHABLE));

		extern const u32 __start__[];
		extern const u32 __text_pages__[];
		extern const u32 __rodata_start__[];
		extern const u32 __rodata_pages__[];
		extern const u32 __data_start__[];
		const u32 dataPages = (AXIWRAM_BASE + AXIWRAM_SIZE - (u32)__data_start__) / 0x1000;

		// text
		mmuMapPages((u32)__start__, (u32)__start__, (u32)__text_pages__,
		            (u32*)(A11_MMU_TABLES_BASE + 0x4400), true, PERM_PRIV_RO_USR_NA, 0, false,
		            L1_TO_L2(CUSTOM_ATTR(POLI_WRITE_BACK_ALLOC_BUFFERED, POLI_WRITE_BACK_ALLOC_BUFFERED)));
		// rodata
		mmuMapPages((u32)__rodata_start__, (u32)__rodata_start__, (u32)__rodata_pages__,
		            (u32*)(A11_MMU_TABLES_BASE + 0x4400), true, PERM_PRIV_RO_USR_NA, 0, true,
		            L1_TO_L2(CUSTOM_ATTR(POLI_WRITE_BACK_ALLOC_BUFFERED, POLI_WRITE_BACK_ALLOC_BUFFERED)));
		// data, bss and heap
		mmuMapPages((u32)__data_start__, (u32)__data_start__, dataPages,
		            (u32*)(A11_MMU_TABLES_BASE + 0x4400u), true, PERM_PRIV_RW_USR_NA, 0, true,
		            L1_TO_L2(CUSTOM_ATTR(POLI_WRITE_BACK_ALLOC_BUFFERED, POLI_WRITE_BACK_ALLOC_BUFFERED)));

		// FCRAM with New 3DS extension
		//mmuMapSupersections(FCRAM_BASE, FCRAM_BASE, 16, PERM_PRIV_RW_USR_NA, true,
		//                    CUSTOM_ATTR(POLI_WRITE_BACK_ALLOC_BUFFERED, POLI_WRITE_BACK_ALLOC_BUFFERED));

		// Map fastboot executable start to boot11 mirror (exception vectors)
		mmuMapPages(BOOT11_MIRROR2, (u32)__start__, 1, (u32*)(A11_MMU_TABLES_BASE + 0x4800u), true,
		            PERM_PRIV_RO_USR_NA, 0, false,
		            L1_TO_L2(CUSTOM_ATTR(POLI_WRITE_BACK_ALLOC_BUFFERED, POLI_WRITE_BACK_ALLOC_BUFFERED)));

		// Invalidate tag RAMs before enabling SMP as recommended by the MPCore doc.
		REG_SCU_CNT = 0x1FFE;        // Disable SCU and parity checking. Access to all CPUs interfaces.
		REG_SCU_INVAL_TAG = 0xFFFF;  // Invalidate SCU tag RAMs of all CPUs.
		REG_SCU_CNT |= 0x2001u;      // Enable SCU and parity checking.

		syncFlag = true;
		__sev();
	}
	else while(!syncFlag) __wfe();


	// Invalidate TLB (Unified TLB operation)
	__asm__ volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (0) : "memory");
	__dsb();


	// Enable Return stack, Dynamic branch prediction, Static branch prediction,
	// Instruction folding, SMP mode: the CPU is taking part in coherency
	// and L1 parity checking
	__setAcr(__getAcr() | 0x6F);

	// Enable MMU, D-Cache, Program flow prediction,
	// I-Cache, high exception vectors, Unaligned data access,
	// subpage AP bits disabled
	__setCr(__getCr() | 0xC03805);

	// Invalidate Both Caches. Also flushes the branch target cache
	__asm__ volatile("mcr p15, 0, %0, c7, c7, 0" : : "r" (0) : "memory");
	__dsb();
	__isb();
}
