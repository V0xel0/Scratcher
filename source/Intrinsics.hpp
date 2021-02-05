#include <intrin.h>

inline auto findFirstSetLSB(u32 value)
{
	unsigned long out = {};
    _BitScanForward((unsigned long *)&out, value);
	return out;
}