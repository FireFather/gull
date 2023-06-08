#include <windows.h>
#include <iostream>
#include "def.h"
#include "macro.h"
#include "const.h"
#include "struct.h"
#include "function.h"
#include "seagull.h"

int sp;
uint64 Stack[2048];
__declspec(align(64)) GBoard Board[1];

void setup_board()
{
	int i = 0;
	uint64 occ = 0;
	GEntry* Entry = NULL;
	GPVEntry* PVEntry = NULL;

	sp = 0;
	date++;

	if (date > 0x8000)
	{
		date = 2;
		for (Entry = Hash, i = 0; i < hash_size; i++, Entry++)
			Entry->date = 1;

		for (PVEntry = PVHash, i = 0; i < pv_hash_size; i++, PVEntry++)
			PVEntry->date = 1;
	}
	Current->material = 0;
	Current->pst = 0;
	Current->key = PieceKey[0][0];

	if (Current->turn)
		Current->key ^= TurnKey;
	Current->key ^= CastleKey[Current->castle_flags];

	if (Current->ep_square)
		Current->key ^= EPKey[File(Current->ep_square)];
	Current->pawn_key = 0;
	Current->pawn_key ^= CastleKey[Current->castle_flags];

	for (i = 0; i < 16; i++)
		BB(i) = 0;

	for (i = 0; i < 64; i++)
	{
		if (Square(i))
		{
			Add(BB(Square(i)), i);
			Add(BB(Square(i) & 1), i);
			Add(occ, i);
			Current->key ^= PieceKey[Square(i)][i];

			if (Square(i) < WhiteKnight)
				Current->pawn_key ^= PieceKey[Square(i)][i];

			if (Square(i) < WhiteKing)
				Current->material += MatCode[Square(i)];
			else
				Current->pawn_key ^= PieceKey[Square(i)][i];
			Current->pst += Pst(Square(i), i);
		}
	}

	if (popcnt(BB(WhiteKnight)) > 2 || popcnt(BB(WhiteLight)) > 1 || popcnt(BB(WhiteDark)) > 1
		|| popcnt(BB(WhiteRook)) > 2 || popcnt(BB(WhiteQueen)) > 2)
		Current->material |= FlagUnusualMaterial;

	if (popcnt(BB(BlackKnight)) > 2 || popcnt(BB(BlackLight)) > 1 || popcnt(BB(BlackDark)) > 1
		|| popcnt(BB(BlackRook)) > 2 || popcnt(BB(BlackQueen)) > 2)
		Current->material |= FlagUnusualMaterial;
	Current->capture = 0;
	Current->killer[1] = Current->killer[2] = 0;
	Current->ply = 0;
	Current->piece_nb = popcnt(PieceAll);
	Stack[sp] = Current->key;
}

void get_board(const char fen[])
{
	int pos = 0, i = 0, j = 0;
	unsigned char c = fen[pos];

	Current = Data;
	memset(Board, 0, sizeof(GBoard));
	memset(Current, 0, sizeof(GData));

	while (c == ' ')
		c = fen[++pos];

	for (i = 56; i >= 0; i -= 8)
	{
		for (j = 0; j <= 7;)
		{
			if (c <= '8')
				j += c - '0';
			else
			{
				Square(i + j) = PieceFromChar[c];

				if (Even(SDiag(i + j)) && (Square(i + j) >> 1) == 3)
					Square(i + j) += 2;
				j++;
			}
			c = fen[++pos];
		}
		c = fen[++pos];
	}

	if (c == 'b')
		Current->turn = 1;
	c = fen[++pos];
	c = fen[++pos];

	if (c == '-')
		c = fen[++pos];

	if (c == 'K')
	{
		Current->castle_flags |= CanCastle_OO;
		c = fen[++pos];
	}

	if (c == 'Q')
	{
		Current->castle_flags |= CanCastle_OOO;
		c = fen[++pos];
	}

	if (c == 'k')
	{
		Current->castle_flags |= CanCastle_oo;
		c = fen[++pos];
	}

	if (c == 'q')
	{
		Current->castle_flags |= CanCastle_ooo;
		c = fen[++pos];
	}
	c = fen[++pos];

	if (c != '-')
	{
		i = c + fen[++pos] * 8 - 489;
		j = i ^ 8;

		if (Square(i) != 0)
			i = 0;

		else if (Square(j) != (3 - Current->turn))
			i = 0;

		else if (Square(j - 1) != (Current->turn + 2) && Square(j + 1) != (Current->turn + 2))
			i = 0;
		Current->ep_square = i;
	}
	setup_board();
}