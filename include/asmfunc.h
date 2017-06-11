#pragma once

#if !__ASSEMBLER__
	#error Only include this in assembly files!
#endif


.macro ASM_FUNC name
	.section .text.\name, "ax", %progbits
	.global \name
	.type \name %function
	.align 2
\name:
.endm
