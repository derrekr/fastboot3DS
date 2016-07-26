#pragma once

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdalign.h>    // alignas(alignment_in_bytes)
#include <stdnoreturn.h> // noreturn keyword



#define PACKED   __attribute__((packed))
#define UNUSED   __attribute__((unused))
#define NAKED    __attribute__((naked))


typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;

typedef volatile int8_t  vs8;
typedef volatile int16_t vs16;
typedef volatile int32_t vs32;
typedef volatile int64_t vs64;
