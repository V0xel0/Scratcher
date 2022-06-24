#pragma once
#include "Utils.hpp"

struct AllocArena
{
	u64 maxSize;
	u64 currOffset;
	u64 prevOffset;
	byte *base;
};

internal void arenaInit(AllocArena *arena, const u64 size, byte *base)
{
	arena->maxSize = size;
	arena->base = base;
	arena->currOffset = 0;
	arena->prevOffset = 0;
}

//! (note)FIXED OUT OF NORMAL PROGRAMMING SESSION!
template<typename T>
[[nodiscard]]
T *arenaPush(AllocArena *arena, const u64 count = 1, const u64 alignment = alignof(T))
{
	GameAssert( ( (arena->currOffset + sizeof(T)*count) <= arena->maxSize) && "No more memory!" );
	arena->currOffset = AlignAddressPow2((u64)arena->base + arena->currOffset, alignment);
	arena->currOffset -= (u64)arena->base;
	T *out = (T*) ((byte*)arena->base + arena->currOffset);
	arena->prevOffset = arena->currOffset;
	arena->currOffset += sizeof(T)*count;
	return out;
}

internal void arenaReset(AllocArena *arena)
{
	GameAssert(arena);
	arena->currOffset = 0;
	arena->prevOffset = 0;
}