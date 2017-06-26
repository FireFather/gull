#include <windows.h>
#include <iostream>
#include "def.h"
#include "macro.h"
#include "const.h"
#include "struct.h"

GMaterial* Material;

static void calc_material(int index)
{
	int pawns[2] = {}, knights[2] = {}, light[2] = {}, dark[2] = {}, rooks[2] = {}, queens[2] = {}, bishops[2] = {};
	int major[2] = {}, minor[2] = {}, tot[2] = {}, mat[2] = {}, mul[2] = {}, quad[2] = {};
	int score = 0, phase = 0, me = 0, i = index;

	queens[White] = i % 3;
	i /= 3;
	queens[Black] = i % 3;
	i /= 3;
	rooks[White] = i % 3;
	i /= 3;
	rooks[Black] = i % 3;
	i /= 3;
	light[White] = i % 2;
	i /= 2;
	light[Black] = i % 2;
	i /= 2;
	dark[White] = i % 2;
	i /= 2;
	dark[Black] = i % 2;
	i /= 2;
	knights[White] = i % 3;
	i /= 3;
	knights[Black] = i % 3;
	i /= 3;
	pawns[White] = i % 9;
	i /= 9;
	pawns[Black] = i % 9;

	for (me = 0; me < 2; me++)
	{
		bishops[me] = light[me] + dark[me];
		major[me] = rooks[me] + queens[me];
		minor[me] = bishops[me] + knights[me];
		tot[me] = 3 * minor[me] + 5 * rooks[me] + 9 * queens[me];
		mat[me] = mul[me] = 32;
		quad[me] = 0;
	}
	score = (SeeValue[WhitePawn] + Av(MatLinear, 0, 0, 0)) * (pawns[White] - pawns[Black])
		+ (SeeValue[WhiteKnight] + Av(MatLinear, 0, 0, 1)) * (knights[White] - knights[Black])
		+ (SeeValue[WhiteLight] + Av(MatLinear, 0, 0, 2)) * (bishops[White] - bishops[Black])
		+ (SeeValue[WhiteRook] + Av(MatLinear, 0, 0, 3)) * (rooks[White] - rooks[Black])
		+ (SeeValue[WhiteQueen] + Av(MatLinear, 0, 0, 4)) * (queens[White] - queens[Black])
		+ BPValue * ((bishops[White] >> 1) - (bishops[Black] >> 1));
	phase = Phase0 * (pawns[White] + pawns[Black])
		+ Phase1 * (knights[White] + knights[Black])
		+ Phase2 * (bishops[White] + bishops[Black])
		+ Phase3 * (rooks[White] + rooks[Black])
		+ Phase4 * (queens[White] + queens[Black]);
	Material[index].phase = Min((Max(phase - PhaseMin, 0) << 7) / (PhaseMax - PhaseMin), 128);

	int special = 0;

	for (me = 0; me < 2; me++)
	{
		if (queens[me] == queens[opp])
		{
			if (rooks[me] - rooks[opp] == 1)
			{
				if (knights[me] == knights[opp] && bishops[opp] - bishops[me] == 1)
					IncV(special, MatRB);

				else if (bishops[me] == bishops[opp] && knights[opp] - knights[me] == 1)
					IncV(special, MatRN);

				else if (knights[me] == knights[opp] && bishops[opp] - bishops[me] == 2)
					DecV(special, MatBBR);

				else if (bishops[me] == bishops[opp] && knights[opp] - knights[me] == 2)
					DecV(special, MatNNR);

				else if (bishops[opp] - bishops[me] == 1 && knights[opp] - knights[me] == 1)
					DecV(special, MatBNR);
			}

			else if (rooks[me] == rooks[opp] && minor[me] - minor[opp] == 1)
				IncV(special, MatM);
		}
		else if (queens[me] - queens[opp] == 1)
		{
			if (rooks[opp] - rooks[me] == 2 && minor[opp] - minor[me] == 0)
				IncV(special, MatQRR);

			else if (rooks[opp] - rooks[me] == 1 && knights[opp] == knights[me] && bishops[opp] - bishops[me] == 1)
				IncV(special, MatQRB);

			else if (rooks[opp] - rooks[me] == 1 && knights[opp] - knights[me] == 1 && bishops[opp] == bishops[me])
				IncV(special, MatQRN);

			else if ((major[opp] + minor[opp]) - (major[me] + minor[me]) >= 2)
				IncV(special, MatQ3);
		}
	}
	score += (Opening(special) * Material[index].phase + Endgame(special) * (128 - (int)Material[index].phase)) >> 7;

	for (me = 0; me < 2; me++)
	{
		quad[me] += pawns[me] * (pawns[me] * TrAv(MatQuadMe, 5, 0, 0) + knights[me] * TrAv(MatQuadMe, 5, 0, 1)
			+ bishops[me] * TrAv(MatQuadMe, 5, 0, 2) + rooks[me] * TrAv(MatQuadMe, 5, 0, 3)
			+ queens[me] * TrAv(MatQuadMe, 5, 0, 4));
		quad[me] += knights[me] * (knights[me] * TrAv(MatQuadMe, 5, 1, 0) + bishops[me] * TrAv(MatQuadMe, 5, 1, 1)
			+ rooks[me] * TrAv(MatQuadMe, 5, 1, 2) + queens[me] * TrAv(MatQuadMe, 5, 1, 3));
		quad[me] += bishops[me] * (bishops[me] * TrAv(MatQuadMe, 5, 2, 0) + rooks[me] * TrAv(MatQuadMe, 5, 2, 1)
			+ queens[me] * TrAv(MatQuadMe, 5, 2, 2));

		quad[me] += rooks[me] * (rooks[me] * TrAv(MatQuadMe, 5, 3, 0) + queens[me] * TrAv(MatQuadMe, 5, 3, 1));
		quad[me] += pawns[me] * (knights[opp] * TrAv(MatQuadOpp, 4, 0, 0) + bishops[opp] * TrAv(MatQuadOpp, 4, 0, 1)
			+ rooks[opp] * TrAv(MatQuadOpp, 4, 0, 2) + queens[opp] * TrAv(MatQuadOpp, 4, 0, 3));
		quad[me] += knights[me] * (bishops[opp] * TrAv(MatQuadOpp, 4, 1, 0) + rooks[opp] * TrAv(MatQuadOpp, 4, 1, 1)
			+ queens[opp] * TrAv(MatQuadOpp, 4, 1, 2));
		quad[me] += bishops[me] * (rooks[opp] * TrAv(MatQuadOpp, 4, 2, 0) + queens[opp] * TrAv(MatQuadOpp, 4, 2, 1));
		quad[me] += rooks[me] * queens[opp] * TrAv(MatQuadOpp, 4, 3, 0);

		if (bishops[me] >= 2)
			quad[me] += pawns[me] * Av(BishopPairQuad, 0, 0, 0) + knights[me] * Av(BishopPairQuad, 0, 0, 1)
			+ rooks[me] * Av(BishopPairQuad, 0, 0, 2) + queens[me] * Av(BishopPairQuad, 0, 0, 3)
			+ pawns[opp] * Av(BishopPairQuad, 0, 0, 4) + knights[opp] * Av(BishopPairQuad, 0, 0, 5)
			+ bishops[opp] * Av(BishopPairQuad, 0, 0, 6) + rooks[opp] * Av(BishopPairQuad, 0, 0, 7)
			+ queens[opp] * Av(BishopPairQuad, 0, 0, 8);
	}
	score += (quad[White] - quad[Black]) / 100;

	for (me = 0; me < 2; me++)
	{
		if (tot[me] - tot[opp] <= 3)
		{
			if (!pawns[me])
			{
				if (tot[me] <= 3)
					mul[me] = 0;

				if (tot[me] == tot[opp] && major[me] == major[opp] && minor[me] == minor[opp])
					mul[me] = major[me] + minor[me] <= 2 ? 0 : (major[me] + minor[me] <= 3 ? 16 : 32);
				else if (minor[me] + major[me] <= 2)
				{
					if (bishops[me] < 2)
						mat[me] = (bishops[me] && rooks[me]) ? 8 : 1;

					else if (bishops[opp] + rooks[opp] >= 1)
						mat[me] = 1;

					else
						mat[me] = 32;
				}

				else if (tot[me] - tot[opp] < 3 && minor[me] + major[me] - minor[opp] - major[opp] <= 1)
					mat[me] = 4;

				else if (minor[me] + major[me] <= 3)
					mat[me] = 8 * (1 + bishops[me]);

				else
					mat[me] = 8 * (2 + bishops[me]);
			}

			if (pawns[me] <= 1)
			{
				mul[me] = Min(28, mul[me]);

				if (rooks[me] == 1 && queens[me] + minor[me] == 0 && rooks[opp] == 1)
					mat[me] = Min(23, mat[me]);
			}
		}

		if (!major[me])
		{
			if (!minor[me])
			{
				if (!tot[me] && pawns[me] < pawns[opp])
					DecV(score, (pawns[opp] - pawns[me]) * SeeValue[WhitePawn]);
			}
			else if (minor[me] == 1)
			{
				if (pawns[me] <= 1 && minor[opp] >= 1)
					mat[me] = 1;

				if (bishops[me] == 1)
				{
					if (minor[opp] == 1 && bishops[opp] == 1 && light[me] != light[opp])
					{
						mul[me] = Min(mul[me], 15);

						if (pawns[me] - pawns[opp] <= 1)
							mul[me] = Min(mul[me], 11);
					}
				}
			}
			else if (!pawns[me] && knights[me] == 2 && !bishops[me])
			{
				if (!tot[opp] && pawns[opp])
					mat[me] = 6;
				else
					mul[me] = 0;
			}
		}

		if (!mul[me])
			mat[me] = 0;

		if (mat[me] <= 1 && tot[me] != tot[opp])
			mul[me] = Min(mul[me], 8);
	}

	if (bishops[White] == 1 && bishops[Black] == 1 && light[White] != light[Black])
	{
		mul[White] = Min(mul[White], 24 + 2 * (knights[Black] + major[Black]));
		mul[Black] = Min(mul[Black], 24 + 2 * (knights[White] + major[White]));
	}
	else if (!minor[White] && !minor[Black] && major[White] == 1 && major[Black] == 1 && rooks[White] == rooks[Black])
	{
		mul[White] = Min(mul[White], 25);
		mul[Black] = Min(mul[Black], 25);
	}

	for (me = 0; me < 2; me++)
	{
		Material[index].mul[me] = mul[me];
		Material[index].pieces[me] = major[me] + minor[me];
	}

	if (score > 0)
		score = (score * mat[White]) >> 5;
	else
		score = (score * mat[Black]) >> 5;
	Material[index].score = score;

	for (me = 0; me < 2; me++)
	{
		if (major[me] == 0 && minor[me] == bishops[me] && minor[me] <= 1)
			Material[index].flags |= VarC(FlagSingleBishop, me);

		if (((major[me] == 0 || minor[me] == 0) && major[me] + minor[me] <= 1) || major[opp] + minor[opp] == 0
			|| (!pawns[me] && major[me] == rooks[me] && major[me] == 1 && minor[me] == bishops[me] && minor[me] == 1
			&& rooks[opp] == 1 && !minor[opp] && !queens[opp]))
			Material[index].flags |= VarC(FlagCallEvalEndgame, me);
	}
}

void init_material()
{
	memset(Material, 0, TotalMat * sizeof(GMaterial));

	for (int index = 0; index < TotalMat; index++)
		calc_material(index);
}