#ifndef BIT_H_INCLUDED
#define BIT_H_INCLUDED

#ifndef W32_BUILD
__forceinline int lsb(uint64 x)
{
	register unsigned long y;
	_BitScanForward64(&y, x);
	return y;
}

__forceinline int msb(uint64 x)
{
	register unsigned long y;
	_BitScanReverse64(&y, x);
	return y;
}

__forceinline int popcnt(uint64 x)
{
	x = x - ((x >> 1) & 0x5555555555555555);
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (x * 0x0101010101010101) >> 56;
}

template <bool HPopCnt> static __forceinline int popcount(uint64 x)
{
	return HPopCnt ? (int)_mm_popcnt_u64(x) : popcnt(x);
}
#else
static __forceinline int lsb(uint64 x)
{
	_asm
	{
		mov eax, dword ptr x[0]
			test eax, eax
			jz l_high
			bsf eax, eax
			jmp l_ret
		l_high : bsf eax, dword ptr x[4]
				 add eax, 20h
			 l_ret :
	}
}

__forceinline int msb(uint64 x)
{
	_asm
	{
		mov eax, dword ptr x[4]
			test eax, eax
			jz l_low
			bsr eax, eax
			add eax, 20h
			jmp l_ret
		l_low : bsr eax, dword ptr x[0]
			l_ret :
	}
}

__forceinline int popcnt(uint64 x)
{
	unsigned int x1, x2;
	x1 = (unsigned int)(x & 0xFFFFFFFF);
	x1 -= (x1 >> 1) & 0x55555555;
	x1 = (x1 & 0x33333333) + ((x1 >> 2) & 0x33333333);
	x1 = (x1 + (x1 >> 4)) & 0x0F0F0F0F;
	x2 = (unsigned int)(x >> 32);
	x2 -= (x2 >> 1) & 0x55555555;
	x2 = (x2 & 0x33333333) + ((x2 >> 2) & 0x33333333);
	x2 = (x2 + (x2 >> 4)) & 0x0F0F0F0F;
	return ((x1 * 0x01010101) >> 24) + ((x2 * 0x01010101) >> 24);
}

template <bool HPopCnt> static __forceinline int popcount(uint64 x)
{
	return HPopCnt ? (__popcnt((int)x) + __popcnt(x >> 32)) : popcnt(x);
}
#endif

#endif