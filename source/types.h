#pragma once
#include <stdint.h>
typedef float f32;
typedef double f64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int32_t s64;

typedef uint8_t byte;

#define KiB(Value) ((Value)*1024LL)
#define MiB(Value) (KiB(Value)*1024LL)
#define GiB(Value) (MiB(Value)*1024LL)
#define TiB(Value) (GiB(Value)*1024LL)

#define internal static
#define local_persist static 
#define global_variable static