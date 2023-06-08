#include <windows.h>
#include <iostream>
#include "def.h"
#include "macro.h"
#include "const.h"
#include "struct.h"
#include "function.h"

uint64 RMask[64];
uint64 BMask[64];
uint64 Between[64][64];
uint64* MagicAttacks;
uint64 BMagicMask[64];
uint64 RMagicMask[64];

uint64 BMagicAttacks(int i, uint64 occ)
{
	uint64 att = 0;

	for (uint64 u = BMask[i]; T(u); Cut(u))
		if (F(Between[i][lsb(u)] & occ))
			att |= Between[i][lsb(u)] | Bit(lsb(u));
	return att;
}

uint64 RMagicAttacks(int i, uint64 occ)
{
	uint64 att = 0;

	for (uint64 u = RMask[i]; T(u); Cut(u))
		if (F(Between[i][lsb(u)] & occ))
			att |= Between[i][lsb(u)] | Bit(lsb(u));
	return att;
}

void init_magic()
{
	int i = 0, j = 0, k = 0, index = 0, bits = 0, bit_list[16] = {};
	uint64 u = 0;

	for (i = 0; i < 64; i++)
	{
		bits = 64 - BShift[i];

		for (u = BMagicMask[i], j = 0; T(u); Cut(u), j++)
			bit_list[j] = lsb(u);

		for (j = 0; j < Bit(bits); j++)
		{
			u = 0;

			for (k = 0; k < bits; k++)
				if (Odd(j >> k))
					Add(u, bit_list[k]);

#ifndef HNI
			index = Convert(BOffset[i] + ((BMagic[i] * u) >> BShift[i]), int);
#else
			index = Convert(BOffset[i] + _pext_u64(u, BMagicMask[i]), int);
#endif

			MagicAttacks[index] = BMagicAttacks(i, u);
		}
		bits = 64 - RShift[i];

		for (u = RMagicMask[i], j = 0; T(u); Cut(u), j++)
			bit_list[j] = lsb(u);

		for (j = 0; j < Bit(bits); j++)
		{
			u = 0;

			for (k = 0; k < bits; k++)
				if (Odd(j >> k))
					Add(u, bit_list[k]);

#ifndef HNI
			index = Convert(ROffset[i] + ((RMagic[i] * u) >> RShift[i]), int);
#else
			index = Convert(ROffset[i] + _pext_u64(u, RMagicMask[i]), int);
#endif

			MagicAttacks[index] = RMagicAttacks(i, u);
		}
	}
}
