#pragma once
#include <cstdint>

using f32 = float;
using f64 = double;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = s32;
using byte = u8;

#define KiB(Value) ((Value)*1024LL)
#define MiB(Value) (KiB(Value)*1024LL)
#define GiB(Value) (MiB(Value)*1024LL)
#define TiB(Value) (GiB(Value)*1024LL)

#define internal static
#define local_persist static 
#define global_variable static

const constexpr f32 PI32 =  3.14159265359f;
const constexpr f64 PI64 =  3.14159265359;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define AlignAddressPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment) - 1))
#define AlignAddress4(Value) ((Value + 3) & ~3)
#define AlignAddress8(Value) ((Value + 7) & ~7)
#define AlignAddress16(Value) ((Value + 15) & ~15)

#define AlignValuePow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment))

#define TestBit(El,Pos) ( (El) & ( 1<<(Pos) ) )