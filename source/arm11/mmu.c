#include "types.h"
#include "mem_map.h"



// Mem permissions
#define PERM_NO_ACC                            (0b000000u)
#define PERM_PRIV_RW_USR_NO_ACC                (0b000001u)
#define PERM_PRIV_RW_USR_RO                    (0b000010u)
#define PERM_PRIV_RW_USR_RW                    (0b000010u)
#define PERM_PRIV_RO_USR_NO_ACC                (0b100001u)
#define PERM_PRIV_RO_USR_RO                    (0b100010u)

// Predefined mem attributes. Only bits 0, 1 and 10-12 matter
#define ATTR_STRONGLY_ORDERED                  (0b0000000000000u) // Always shared
#define ATTR_SHARED_DEVICE                     (0b0000000000001u) // Always shared
#define ATTR_NORM_WRITE_TROUGH_NO_ALLOC        (0b0000000000010u)
#define ATTR_NORM_WRITE_BACK_NO_ALLOC          (0b0000000000011u)
#define ATTR_NORM_NONCACHABLE                  (0b0010000000000u)
#define ATTR_NORM_WRITE_BACK_ALLOC             (0b0010000000011u)
#define ATTR_NONSHARED_DEVICE                  (0b0100000000000u) // Always non-shared

// Policies for custom normal mem attributes
#define POLICY_NONCACHABLE_UNBUFFERED          (0b00u)
#define POLICY_WRITE_BACK_ALLOC_BUFFERED       (0b01u)
#define POLICY_WRITE_THROUGH_NO_ALLOC_BUFFERED (0b10u) // Behaves as noncacheable on ARM11 MPCore
#define POLICY_WRITE_BACK_NO_ALLOC_BUFFERED    (0b11u)

#define MAKE_CUSTOM_NORM_ATTR(outer, inner)    ((1u<<12) | (outer<<10) | (inner))

// Converts the attribute bits from L1 format to L2 format
#define L1_TO_L2(attr)                         ((attr>>6 | attr) & 0x73)



extern void clearMem(u32 *adr, u32 size);
extern u32 __start__[];

/**
 * @brief      Maps up to 4096 1 MB sections of memory.
 *
 * @param[in]  va      The virtual address base. Must be aligned to 1 MB.
 * @param[in]  pa      The physical address base. Must be aligned to 1 MB.
 * @param[in]  num     The number of sections to map.
 * @param[in]  shared  If the sections are shared memory.
 * @param[in]  access  The access permission bits.
 * @param[in]  domain  One of the 16 possible domains.
 * @param[in]  xn      If this memory should be marked as execute never.
 * @param[in]  attr    Other attribute bits like caching.
 */
static void mmuMapSections(u32 va, u32 pa, u32 num, bool shared, u32 access, u8 domain, bool xn, u32 attr)
{
	for(u32 i = 0; i < 0x100000u * num; i += 0x100000u)
	{
		((u32*)A11_MMU_TABLES_BASE)[(va + i)>>20] = (((pa + i) & 0xFFF00000u) | (u32)shared<<16 | access<<10 |
		                                             (u32)domain<<5 | (u32)xn<<4 | attr<<2 | 0b10u);
	}
}

/**
 * @brief      Maps up to 128 4 KB pages of memory.
 * @brief      The mapped range must not cross the next section.
 *
 * @param[in]  va       The virtual address base. Must be aligned to 4 KB.
 * @param[in]  pa       The physical address base. Must be aligned to 4 KB.
 * @param[in]  num      The number of pages to map. Must be <= 128.
 * @param      l2Table  The L2 MMU table address base for this mapping.
 * @param[in]  shared   If the pages are shared memory.
 * @param[in]  access   The access permission bits.
 * @param[in]  domain   One of the 16 possible domains.
 * @param[in]  xn       If this memory should be marked as execute never.
 * @param[in]  attr     Other attribute bits like caching.
 */
static void mmuMapPages(u32 va, u32 pa, u32 num, u32 *l2Table, bool shared, u32 access, u8 domain, bool xn, u32 attr)
{
	((u32*)A11_MMU_TABLES_BASE)[va>>20] = (((u32)l2Table & 0xFFFFFC00u) | (u32)domain<<5 | 0b01u);

	for(u32 i = 0; i < 0x1000u * num; i += 0x1000u)
	{
		l2Table[(va + i)>>12 & 0xFFu] = (((pa + i) & 0xFFFFF000u) | (u32)shared<<10 |
		                                 access<<4 | attr<<2 | 0b10u | (u32)xn);
	}
}

void setupMmu(void)
{
	// TTBR0 address shared page table walk and outer cachable write-through, no allocate on write
	__asm__ __volatile__("mcr p15, 0, %0, c2, c0, 0" : : "r" (A11_MMU_TABLES_BASE | 0x12u));
	// Use the 16 KB L1 table only
	__asm__ __volatile__("mcr p15, 0, %0, c2, c0, 2" : : "r" (0u));
	// Domain 1 = client, remaining domains all = no access
	__asm__ __volatile__("mcr p15, 0, %0, c3, c0, 0" : : "r" (4u));
	// Context ID Register (ASID = 0, PROCID = 0)
	__asm__ __volatile__("mcr p15, 0, %0, c13, c0, 1" : : "r" (0u));


	// Clear L1 and L2 tables
	clearMem((u32*)A11_MMU_TABLES_BASE, 0x4C00u);

	// IO mem mapping
	mmuMapSections(IO_MEM_ARM9_ARM11, IO_MEM_ARM9_ARM11, 4, true,
	               PERM_PRIV_RW_USR_NO_ACC, 1, true, ATTR_SHARED_DEVICE);

	// MPCore private region mapping
	mmuMapPages(MPCORE_PRIV_REG_BASE, MPCORE_PRIV_REG_BASE, 2,
	            (u32*)(A11_MMU_TABLES_BASE + 0x4000u), false, PERM_PRIV_RW_USR_NO_ACC,
	            1, true, L1_TO_L2(ATTR_NONSHARED_DEVICE));

	// VRAM mapping
	mmuMapSections(VRAM_BASE, VRAM_BASE, 6, true, PERM_PRIV_RW_USR_NO_ACC, 1, true, ATTR_NORM_WRITE_TROUGH_NO_ALLOC);

	// AXIWRAM MMU table mapping
	mmuMapPages(A11_MMU_TABLES_BASE, A11_MMU_TABLES_BASE, 5, (u32*)(A11_MMU_TABLES_BASE + 0x4400u), true,
	            PERM_PRIV_RO_USR_NO_ACC, 1, true, L1_TO_L2(ATTR_NORM_NONCACHABLE));

	// Remaining AXIWRAM pages
	mmuMapPages(AXIWRAM_BASE + 0x5000u, AXIWRAM_BASE + 0x5000u, 123,
	            (u32*)(A11_MMU_TABLES_BASE + 0x4400u), true, PERM_PRIV_RW_USR_NO_ACC, 1, false,
	            L1_TO_L2(MAKE_CUSTOM_NORM_ATTR(POLICY_WRITE_BACK_ALLOC_BUFFERED, POLICY_WRITE_BACK_ALLOC_BUFFERED)));

	// Map boot11 mirror to loader executable start (exception vectors)
	mmuMapPages(BOOT11_MIRROR2, (u32)__start__, 1, (u32*)(A11_MMU_TABLES_BASE + 0x4800u), true,
	            PERM_PRIV_RO_USR_NO_ACC, 1, false,
	            L1_TO_L2(MAKE_CUSTOM_NORM_ATTR(POLICY_WRITE_BACK_ALLOC_BUFFERED, POLICY_WRITE_BACK_ALLOC_BUFFERED)));


	// Invalidate TLB (Unified TLB operation) + Data Synchronization Barrier
	__asm__ __volatile__("mcr p15, 0, %0, c8, c7, 0\n\t"
	                     "mcr p15, 0, %0, c7, c10, 4" : : "r" (0u));


	// Invalidate tag RAMs before enabling SMP like recommended by the MPCore docs
	((vu32*)MPCORE_PRIV_REG_BASE)[0] = 0u;      // disable SCU
	((vu32*)MPCORE_PRIV_REG_BASE)[3] = 0xFFFFu; // Invalidate alll SCU tag RAMs of all CPUs
	((vu32*)MPCORE_PRIV_REG_BASE)[0] = 0x3FFFu; // Enable SCU and parity checking. All CPUs can access SCU regs
	                                            // and CPU interrupt/peripheral interfaces.


	u32 tmp;
	// Modify Auxiliary Control Register
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1" : "=r" (tmp) : );
	tmp |= 0x6Fu;     // Enable Return stack, Dynamic branch prediction, Static branch prediction,
	                  // Instruction folding, SMP mode: the CPU is taking part in coherency
	                  // and L1 parity checking
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1" : : "r" (tmp));

	// Modify Control Register
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r" (tmp) : );
	tmp |= 0xC03805u; // Enable MMU, D-Cache, Program flow prediction,
	                  // I-Cache, high exception vectors, Unaligned data access,
	                  // subpage AP bits disabled
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : : "r" (tmp));

	// Invalidate all caches + Data Synchronization Barrier
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 4\n\t"
	                     "mcr p15, 0, %0, c7, c7, 0\n\t"
	                     "mcr p15, 0, %0, c7, c10, 4" : : "r" (0u));
}
