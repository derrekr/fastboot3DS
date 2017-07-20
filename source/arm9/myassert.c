#include "types.h"
#include "arm9/fmt.h"
#include "arm9/interrupt.h"



noreturn void __myassert(const char *const str, u32 line)
{
	ee_printf("Assertion failed: %s:%" PRIu32, str, line);

	while(1) waitForInterrupt();
}
