#pragma once
#include <cstdint>
#include "GameAssertions.hpp"

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

using b8 = s8;
using b16 = s16;
using b32 = s32;

using byte = u8;

#define KiB(Value) ((Value)*1024LL)
#define MiB(Value) (KiB(Value) * 1024LL)
#define GiB(Value) (MiB(Value) * 1024LL)
#define TiB(Value) (GiB(Value) * 1024LL)

#define internal static
#define local_persist static
#define global_variable static

constexpr f32 PI32 = 3.14159265359f;
constexpr f64 PI64 = 3.14159265359;

template <typename F>
struct DummyDefer
{
	F f;
	DummyDefer(F f) : f(f) {}
	~DummyDefer() { f(); }
};

template <typename F>
DummyDefer<F> deferFunction(F f)
{
	return DummyDefer<F>(f);
}

u32 truncU64toU32(u64 val)
{
	GameAssert(val <= 0xffffffff);
	u32 Result = (u32)val;
	return (Result);
};

template <typename T, s64 N>
constexpr s64 ArrayCount64(const T (&array)[N]) noexcept
{
	return N;
}

template <typename T, s32 N>
constexpr s32 ArrayCount32(const T (&array)[N]) noexcept
{
	return N;
}

template <typename T>
void swap(T &a, T &b)
{
    T temp(static_cast<T&&>(a));
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(temp);
}

template<typename T>
void pointerSwap(T **a, T **b)
{
    T *temp = *a;
    *a = *b;
    *b = temp;
}

template<typename T>
void valueSwap(T *a, T *b)
{
    T temp = *a;
    *a = *b;
    *b = temp;
}

template<typename T>
void bitSwap(T *a, T *b)
{
	a = a ^ b;
	b = a ^ b;
	a = a ^ b;
}

template <typename ...T>
auto min(T&&... args)
{
	auto min = (args, ...);
	((args < min ? min = args : 0), ...);
    return min;
}

template <typename ...T>
auto max(T&&... args)
{
	auto max = (args, ...);
	((args > max ? max = args : 0), ...);
    return max;
}

template <typename T>
T clamp(T x, T low, T high)
{
	return min(max(low, x), high);
}

#define AlignAddressPow2(Value, Alignment) ((Value) + ((Alignment)-1)) & ~((Alignment)-1)
#define AlignAddress4(Value) ((Value + 3) & ~3)
#define AlignAddress8(Value) ((Value + 7) & ~7)
#define AlignAddress16(Value) ((Value + 15) & ~15)

#define AlignValuePow2(Value, Alignment) ((Value) + ((Alignment)-1)) & -(Alignment)

#define TestBit(El, Pos) ((El) & (1 << (Pos)))