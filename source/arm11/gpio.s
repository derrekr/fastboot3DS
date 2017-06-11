#include "asmfunc.h"

.arm
.cpu mpcore
.fpu vfpv2



ASM_FUNC gpio_set_bit
	ldrh r2, [r0]
	movs r3, #1
	movs r1, r3, lsl r1
	orrs r2, r1
	strh r2, [r0]
	bx lr


ASM_FUNC gpio_clear_bit
	ldrh r2, [r0]
	movs r3, #1
	movs r1, r3, lsl r1
	bics r2, r1
	strh r2, [r0]
	bx lr
