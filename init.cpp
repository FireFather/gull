#include <windows.h>
#include <iostream>
#include "def.h"
#include "macro.h"
#include "const.h"
#include "struct.h"
#include "function.h"
#include "kpk.h"
#include "seagull.h"

uint64 Forward[2][8];
uint64 West[8];
uint64 East[8];
uint64 PIsolated[8];
uint64 HLine[64];
uint64 VLine[64];
uint64 NDiag[64];
uint64 SDiag[64];

uint64 QMask[64];
uint64 NAtt[64];
uint64 SArea[64];
uint64 DArea[64];
uint64 NArea[64];
uint64 BishopForward[2][64];
uint64 PAtt[2][64];
uint64 PMove[2][64];
uint64 PWay[2][64];
uint64 PSupport[2][64];
uint64 FullLine[64][64];

uint8 PieceFromChar[256];
uint64 PieceKey[16][64];
uint64 TurnKey;
uint64 CastleKey[16];
uint64 EPKey[8];

uint64* BOffsetPointer[64];
uint64* ROffsetPointer[64];

int MvvLva[16][16];
static uint64 seed = 1;
int Pst[16 * 64];

void init()
{
	init_shared();
	init_misc();
	
	if (parent)
		init_magic();

	for (int i = 0; i < 64; i++)
	{
		BOffsetPointer[i] = MagicAttacks + BOffset[i];
		ROffsetPointer[i] = MagicAttacks + ROffset[i];
	}
	gen_kpk();
	init_pst();
	init_eval();

	if (parent)
		init_material();
}

void init_eval()
{
	int i = 0, j = 0, k = 0, index = 0;
	memset(Mobility, 0, 4 * 32 * sizeof(int));

	for (i = 0; i < 4; i++)
		for (j = 0; j < 32; j++)
		{
			index = i << 1;
			double op = (double)(Av(MobilityLinear, 8, 0, index) * j)
				+ log(1.01 + (double)j) * (double)(Av(MobilityLog, 8, 0, index));
			index = (i << 1) + 1;
			double eg = (double)(Av(MobilityLinear, 8, 0, index) * j)
				+ log(1.01 + (double)j) * (double)(Av(MobilityLog, 8, 0, index));
			Mobility[i][j] = Compose((int)(op / 64.0), (int)(eg / 64.0));
		}

	for (i = 0; i < 3; i++)
		for (j = 7; j >= 0; j--)
		{
			Shelter[i][j] = 0;

			if (j > 1)
				for (k = 1; k < Min(j, 5); k++)
					Shelter[i][j] += Av(ShelterValue, 0, 0, (i * 5) + k - 1);

			if (!j)
				Shelter[i][j] = Shelter[i][7] + Av(ShelterValue, 0, 0, (i * 5) + 4);
		}

	for (i = 0; i < 4; i++)
	{
		StormBlocked[i] =
			((StormBlockedMulQuad * i * i) + (StormBlockedMulLinear * (i + 1))) / 100;
		StormShelterAtt[i] =
			((StormShelterAttMulQuad * i * i) + (StormShelterAttMulLinear * (i + 1))) / 100;
		StormConnected[i] =
			((StormConnectedMulQuad * i * i) + (StormConnectedMulLinear * (i + 1))) / 100;
		StormOpen[i] = ((StormOpenMulQuad * i * i) + (StormOpenMulLinear * (i + 1))) / 100;
		StormFree[i] = ((StormFreeMulQuad * i * i) + (StormFreeMulLinear * (i + 1))) / 100;
	}

	for (i = 0; i < 8; i++)
	{
		int l = Max(i - 2, 0);
		int q = l * l;
		PasserGeneral[i] = Compose16(Av(PasserQuad, 2, 0, 0) * q + Av(PasserLinear, 2, 0, 0) * l,
			Av(PasserQuad, 2, 0, 1) * q + Av(PasserLinear, 2, 0, 1) * l);
		PasserBlocked[i] = Compose16(Av(PasserQuad, 2, 1, 0) * q + Av(PasserLinear, 2, 1, 0) * l,
			Av(PasserQuad, 2, 1, 1) * q + Av(PasserLinear, 2, 1, 1) * l);
		PasserFree[i] = Compose16(Av(PasserQuad, 2, 2, 0) * q + Av(PasserLinear, 2, 2, 0) * l,
			Av(PasserQuad, 2, 2, 1) * q + Av(PasserLinear, 2, 2, 1) * l);
		PasserSupported[i] = Compose16(Av(PasserQuad, 2, 3, 0) * q + Av(PasserLinear, 2, 3, 0) * l,
			Av(PasserQuad, 2, 3, 1) * q + Av(PasserLinear, 2, 3, 1) * l);
		PasserProtected[i] = Compose16(Av(PasserQuad, 2, 4, 0) * q + Av(PasserLinear, 2, 4, 0) * l,
			Av(PasserQuad, 2, 4, 1) * q + Av(PasserLinear, 2, 4, 1) * l);
		PasserConnected[i] = Compose16(Av(PasserQuad, 2, 5, 0) * q + Av(PasserLinear, 2, 5, 0) * l,
			Av(PasserQuad, 2, 5, 1) * q + Av(PasserLinear, 2, 5, 1) * l);
		PasserOutside[i] = Compose16(Av(PasserQuad, 2, 6, 0) * q + Av(PasserLinear, 2, 6, 0) * l,
			Av(PasserQuad, 2, 6, 1) * q + Av(PasserLinear, 2, 6, 1) * l);
		PasserCandidate[i] = Compose16(Av(PasserQuad, 2, 7, 0) * q + Av(PasserLinear, 2, 7, 0) * l,
			Av(PasserQuad, 2, 7, 1) * q + Av(PasserLinear, 2, 7, 1) * l);
		PasserClear[i] = Compose16(Av(PasserQuad, 2, 8, 0) * q + Av(PasserLinear, 2, 8, 0) * l,
			Av(PasserQuad, 2, 8, 1) * q + Av(PasserLinear, 2, 8, 1) * l);

		PasserAtt[i] = Av(PasserAttDefQuad, 2, 0, 0) * q + Av(PasserAttDefLinear, 2, 0, 0) * l;
		PasserDef[i] = Av(PasserAttDefQuad, 2, 1, 0) * q + Av(PasserAttDefLinear, 2, 1, 0) * l;
		PasserAttLog[i] = Av(PasserAttDefQuad, 2, 0, 1) * q + Av(PasserAttDefLinear, 2, 0, 1) * l;
		PasserDefLog[i] = Av(PasserAttDefQuad, 2, 1, 1) * q + Av(PasserAttDefLinear, 2, 1, 1) * l;
	}
}

void init_pst()
{
	int i, j, k, op, eg, index, r, f, d, e, distQ[4], distL[4], distM[2];
	memset(Pst, 0, 16 * 64 * sizeof(int));

	for (i = 0; i < 64; i++)
	{
		r = Rank(i);
		f = File(i);
		d = Abs(f - r);
		e = Abs(f + r - 7);
		distQ[0] = DistC[f] * DistC[f];
		distL[0] = DistC[f];
		distQ[1] = DistC[r] * DistC[r];
		distL[1] = DistC[r];
		distQ[2] = RankR[d] * RankR[d] + RankR[e] * RankR[e];
		distL[2] = RankR[d] + RankR[e];
		distQ[3] = RankR[r] * RankR[r];
		distL[3] = RankR[r];
		distM[0] = DistC[f] * DistC[r];
		distM[1] = DistC[f] * RankR[r];

		for (j = 2; j < 16; j += 2)
		{
			index = PieceType[j];
			op = eg = 0;

			for (k = 0; k < 2; k++)
			{
				op += Av(PstQuadMixedWeights, 4, index, (k * 2)) * distM[k];
				eg += Av(PstQuadMixedWeights, 4, index, (k * 2) + 1) * distM[k];
			}

			for (k = 0; k < 4; k++)
			{
				op += Av(PstQuadWeights, 8, index, (k * 2)) * distQ[k];
				eg += Av(PstQuadWeights, 8, index, (k * 2) + 1) * distQ[k];
				op += Av(PstLinearWeights, 8, index, (k * 2)) * distL[k];
				eg += Av(PstLinearWeights, 8, index, (k * 2) + 1) * distL[k];
			}
			Pst(j, i) = Compose(op / 64, eg / 64);
		}
	}

	Pst(WhiteKnight, 56) -= Compose(100, 0);
	Pst(WhiteKnight, 63) -= Compose(100, 0);

	for (i = 0; i < 64; i++)
	{
		for (j = 3; j < 16; j += 2)
		{
			op = Opening(Pst(j - 1, 63 - i));
			eg = Endgame(Pst(j - 1, 63 - i));
			Pst(j, i) = Compose(-op, -eg);
		}
	}
	Current->pst = 0;

	for (i = 0; i < 64; i++)
		if (Square(i))
			Current->pst += Pst(Square(i), i);
}

void gen_kpk()
{
	int turn = 0, wp = 0, wk = 0, bk = 0;

	for (turn = 0; turn < 2; turn++)
	{
		for (wp = 0; wp < 64; wp++)
		{
			for (wk = 0; wk < 64; wk++)
			{
				Kpk[turn][wp][wk] = 0;

				for (bk = 0; bk < 64; bk++)
				{
					if (Kpk_gen[turn][wp][wk][bk] == 2)
						Kpk[turn][wp][wk] |= Bit(bk);
				}
			}
		}
	}
}

static uint16 rand16()
{
	seed = (seed * Convert(6364136223846793005, uint64)) + Convert(1442695040888963407, uint64);
	return Convert((seed >> 32) & 0xFFFF, uint16);
}

static uint64 random()
{
	uint64 key = Convert(rand16(), uint64);
	key <<= 16;
	key |= Convert(rand16(), uint64);
	key <<= 16;
	key |= Convert(rand16(), uint64);
	key <<= 16;
	return key | Convert(rand16(), uint64);
}

void init_misc()
{
	int i = 0, j = 0, k = 0, l = 0, n = 0;
	uint64 u = 0;

	for (i = 0; i < 64; i++)
	{
		HLine[i] = VLine[i] = NDiag[i] = SDiag[i] = RMask[i] = BMask[i] = QMask[i] = 0;
		BMagicMask[i] = RMagicMask[i] = NAtt[i] = SArea[i] = DArea[i] = NArea[i] = 0;
		PAtt[0][i] = PAtt[1][i] = PMove[0][i] = PMove[1][i] = PWay[0][i] = PWay[1][i] = PSupport[0][i] = PSupport[1][i]
			= BishopForward[0][i] = BishopForward[1][i] = 0;

		for (j = 0; j < 64; j++)
			Between[i][j] = FullLine[i][j] = 0;
	}

	for (i = 0; i < 64; i++)
		for (j = 0; j < 64; j++)
			if (i != j)
			{
				u = Bit(j);

				if (File(i) == File(j))
					VLine[i] |= u;

				if (Rank(i) == Rank(j))
					HLine[i] |= u;

				if (NDiag(i) == NDiag(j))
					NDiag[i] |= u;

				if (SDiag(i) == SDiag(j))
					SDiag[i] |= u;

				if (Dist(i, j) <= 2)
				{
					DArea[i] |= u;

					if (Dist(i, j) <= 1)
						SArea[i] |= u;

					if (Abs(Rank(i) - Rank(j)) + Abs(File(i) - File(j)) == 3)
						NAtt[i] |= u;
				}

				if (j == i + 8)
					PMove[0][i] |= u;

				if (j == i - 8)
					PMove[1][i] |= u;

				if (Abs(File(i) - File(j)) == 1)
				{
					if (Rank(j) >= Rank(i))
					{
						PSupport[1][i] |= u;

						if (Rank(j) - Rank(i) == 1)
							PAtt[0][i] |= u;
					}

					if (Rank(j) <= Rank(i))
					{
						PSupport[0][i] |= u;

						if (Rank(i) - Rank(j) == 1)
							PAtt[1][i] |= u;
					}
				}
				else if (File(i) == File(j))
				{
					if (Rank(j) > Rank(i))
						PWay[0][i] |= u;
					else
						PWay[1][i] |= u;
				}
			}

	for (i = 0; i < 64; i++)
	{
		RMask[i] = HLine[i] | VLine[i];
		BMask[i] = NDiag[i] | SDiag[i];
		QMask[i] = RMask[i] | BMask[i];
		BMagicMask[i] = BMask[i] & Interior;
		RMagicMask[i] = RMask[i];

		if (File(i) > 0)
			RMagicMask[i] &= ~File[0];

		if (Rank(i) > 0)
			RMagicMask[i] &= ~Line[0];

		if (File(i) < 7)
			RMagicMask[i] &= ~File[7];

		if (Rank(i) < 7)
			RMagicMask[i] &= ~Line[7];

		for (j = 0; j < 64; j++)
			if (NAtt[i] & NAtt[j])
				Add(NArea[i], j);
	}

	for (i = 0; i < 8; i++)
	{
		West[i] = 0;
		East[i] = 0;
		Forward[0][i] = Forward[1][i] = 0;
		PIsolated[i] = 0;

		for (j = 0; j < 8; j++)
		{
			if (i < j)
				Forward[0][i] |= Line[j];

			else if (i > j)
				Forward[1][i] |= Line[j];

			if (i < j)
				East[i] |= File[j];

			else if (i > j)
				West[i] |= File[j];
		}

		if (i > 0)
			PIsolated[i] |= File[i - 1];

		if (i < 7)
			PIsolated[i] |= File[i + 1];
	}

	for (i = 0; i < 64; i++)
	{
		for (u = QMask[i]; T(u); Cut(u))
		{
			j = lsb(u);
			k = Sgn(Rank(j) - Rank(i));
			l = Sgn(File(j) - File(i));

			for (n = i + 8 * k + l; n != j; n += (8 * k + l))
				Add(Between[i][j], n);
		}

		for (u = BMask[i]; T(u); Cut(u))
		{
			j = lsb(u);
			FullLine[i][j] = BMask[i] & BMask[j];
		}

		for (u = RMask[i]; T(u); Cut(u))
		{
			j = lsb(u);
			FullLine[i][j] = RMask[i] & RMask[j];
		}
		BishopForward[0][i] |= PWay[0][i];
		BishopForward[1][i] |= PWay[1][i];

		for (j = 0; j < 64; j++)
		{
			if ((PWay[1][j] | Bit(j)) & BMask[i] & Forward[0][Rank(i)])
				BishopForward[0][i] |= Bit(j);

			if ((PWay[0][j] | Bit(j)) & BMask[i] & Forward[1][Rank(i)])
				BishopForward[1][i] |= Bit(j);
		}
	}

	for (i = 0; i < 16; i++)
		for (j = 0; j < 16; j++)
		{
			if (j < WhitePawn)
				MvvLva[i][j] = 0;

			else if (j < WhiteKnight)
				MvvLva[i][j] = PawnCaptureMvvLva(i) << 26;

			else if (j < WhiteLight)
				MvvLva[i][j] = KnightCaptureMvvLva(i) << 26;

			else if (j < WhiteRook)
				MvvLva[i][j] = BishopCaptureMvvLva(i) << 26;

			else if (j < WhiteQueen)
				MvvLva[i][j] = RookCaptureMvvLva(i) << 26;

			else
				MvvLva[i][j] = QueenCaptureMvvLva(i) << 26;
		}

	for (i = 0; i < 256; i++)
		PieceFromChar[i] = 0;
	PieceFromChar[66] = 6;
	PieceFromChar[75] = 14;
	PieceFromChar[78] = 4;
	PieceFromChar[80] = 2;
	PieceFromChar[81] = 12;
	PieceFromChar[82] = 10;
	PieceFromChar[98] = 7;
	PieceFromChar[107] = 15;
	PieceFromChar[110] = 5;
	PieceFromChar[112] = 3;
	PieceFromChar[113] = 13;
	PieceFromChar[114] = 11;

	TurnKey = random();

	for (i = 0; i < 8; i++)
		EPKey[i] = random();

	for (i = 0; i < 16; i++)
		CastleKey[i] = random();

	for (i = 0; i < 16; i++)
		for (j = 0; j < 64; j++)
		{
			if (i == 0)
				PieceKey[i][j] = 0;
			else
				PieceKey[i][j] = random();
		}

	for (i = 0; i < 16; i++)
		LogDist[i] = (int)(10.0*log(1.01 + (double)i));
}
