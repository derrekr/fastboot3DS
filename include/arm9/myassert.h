#pragma once

#include "types.h"



noreturn void __myassert(const char *const str, u32 line);

#ifdef NDEBUG
#define myassert(c) ((void)0)
#else
#define myassert(c) ((c) ? ((void)0) : __myassert(#c ", " __FILE__, __LINE__))
#endif
