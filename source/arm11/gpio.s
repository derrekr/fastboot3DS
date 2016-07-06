.arm
.cpu mpcore
.fpu softvfp

.global gpio_set_bit
.global gpio_clear_bit

.type gpio_set_bit STT_FUNC
.type gpio_clear_bit STT_FUNC

.section ".text"



gpio_set_bit:
	ldrh r2, [r0]
	movs r3, #1
	movs r1, r3, lsl r1
	orrs r2, r1
	strh r2, [r0]
	bx lr


gpio_clear_bit:
	ldrh r2, [r0]
	movs r3, #1
	movs r1, r3, lsl r1
	bics r2, r1
	strh r2, [r0]
	bx lr
