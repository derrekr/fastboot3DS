#include "types.h"
#include "mem_map.h"
#include "arm11/firm_launch_stub.h"



void firmLaunchStub(void)
{
	// Answer ARM0
	*((vu32*)CORE_SYNC_ID) = 0x4F4B4F4B;

	// Wait for entry address
	while(!*((vu32*)CORE_SYNC_PARAM));

	// Tell ARM9 we got the entry
	*((vu32*)CORE_SYNC_ID) = 0x544F4F42;

	void (*arm11_entry)(void) = (void*)*((vu32*)CORE_SYNC_PARAM);
	arm11_entry();
}
