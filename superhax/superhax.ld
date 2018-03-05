OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_vectors)

PHDRS
{
	code   PT_LOAD FLAGS(5) /* Read | Execute */;
}

SECTIONS
{
	/* =========== CODE section =========== */

	PROVIDE(__start__ = 0x07FFFE8C);
	. = __start__;

	.text ALIGN(4) :
	{
		/* .init */
		KEEP( *(.crt0) )
		. = ALIGN(4);
	} : code = 0xFF
}
