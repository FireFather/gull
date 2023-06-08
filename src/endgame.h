#ifndef ENDGAME_H_INCLUDED
#define ENDGAME_H_INCLUDED

uint64 Kpk[2][64][64];

static __forceinline int MinF(int x, int y)
{
	return Min(x, y);
}

static __forceinline double MinF(double x, double y)
{
	return Min(x, y);
}

template <int me, bool HPopCnt> static void eval_endgame(GEvalInfo &EI)
{
	if ((EI.material->flags & VarC(FlagSingleBishop, me)) && Pawn(me))
	{
		int sq = (Board->bb[ILight(me)] ? (me ? 0 : 63)
			: (Board->bb[IDark(me)] ? (me ? 7 : 56) : (File(lsb(King(opp))) <= 3 ? (me ? 0 : 56) : (me ? 7 : 63))));

		if (!(Pawn(me) & (~PWay[opp][sq])))
		{
			if ((SArea[sq] | Bit(sq)) & King(opp))
				EI.mul = 0;

			else if ((SArea[sq] & SArea[lsb(King(opp))] & Line(me, 7)) && Square(sq - Push(me)) == IPawn(opp)
				&& Square(sq - 2 * Push(me)) == IPawn(me))
				EI.mul = 0;
		}

		else if ((King(opp) & Line(me, 6) | Line(me, 7)) && Abs(File(sq) - File(lsb(King(opp)))) <= 3
			&& !(Pawn(me) & (~PSupport[me][sq])) && (Pawn(me) & Line(me, 5) & Shift(opp, Pawn(opp))))
			EI.mul = 0;

		if (Single(Pawn(me)))
		{
			if (!Bishop(me))
			{
				EI.mul = MinF(EI.mul, kpkx<me>());

				if (Piece(opp) == King(opp) && EI.mul == 32)
					IncV(Current->score, KpkValue);
			}
			else
			{
				sq = lsb(Pawn(me));

				if ((Pawn(me) & (File[1] | File[6]) & Line(me, 5)) && Square(sq + Push(me)) == IPawn(opp)
					&& ((PAtt[me][sq + Push(me)] | PWay[me][sq + Push(me)]) & King(opp)))
					EI.mul = 0;
			}
		}

		if (Bishop(opp) && Single(Bishop(opp)) && T(BB(ILight(me))) != T(BB(ILight(opp))))
		{
			int pcnt = 0;

			if (T(King(opp) & LightArea) == T(Bishop(opp) & LightArea))
			{
				for (uint64 u = Pawn(me); u; Cut(u))
				{
					if (pcnt >= 2)
						goto check_for_partial_block;

					pcnt++;
					int sq = lsb(u);

					if (!(PWay[me][sq] & (PAtt[me][PVarC(EI, king, opp)] | PAtt[opp][PVarC(EI, king, opp)])))
					{
						if (!(PWay[me][sq] & Pawn(opp)))
							goto check_for_partial_block;
						int bsq = lsb(Bishop(opp));
						uint64 att = BishopAttacks(bsq, EI.occ);

						if (!(att & PWay[me][sq] & Pawn(opp)))
							goto check_for_partial_block;

						if (!(BishopForward[me][bsq] & att & PWay[me][sq] & Pawn(opp))
							&& popcount<HPopCnt>(FullLine[lsb(att & PWay[me][sq] & Pawn(opp))][bsq] & att) <= 2)
							goto check_for_partial_block;
					}
				}
				EI.mul = 0;
				return;
			}
		check_for_partial_block:
			if (pcnt <= 2 && Multiple(Pawn(me)) && !Pawn(opp) && !(Pawn(me) & Boundary) && EI.mul)
			{
				int sq1 = lsb(Pawn(me));
				int sq2 = msb(Pawn(me));
				int fd = Abs(File(sq2) - File(sq1));

				if (fd >= 5)
					EI.mul = 32;

				else if (fd >= 4)
					EI.mul = 26;

				else if (fd >= 3)
					EI.mul = 20;
			}

			if ((SArea[PVarC(EI, king, opp)] | Current->patt[opp]) & Bishop(opp))
			{
				uint64 push = Shift(me, Pawn(me));

				if (!(push &(~(Piece(opp) | Current->att[opp])))
					&& (King(opp) & (Board->bb[ILight(opp)] ? LightArea : DarkArea)))
				{
					EI.mul = Min(EI.mul, 8);
					int bsq = lsb(Bishop(opp));
					uint64 att = BishopAttacks(bsq, EI.occ);
					uint64 prp = (att | SArea[PVarC(EI, king, opp)]) & Pawn(opp) & (Board->bb[ILight(opp)]
						? LightArea : DarkArea);
					uint64 patt = ShiftW(opp, prp) | ShiftE(opp, prp);

					if ((SArea[PVarC(EI, king, opp)] | patt) & Bishop(opp))
					{
						uint64 double_att =
							(SArea[PVarC(EI, king, opp)] & patt) | (patt & att) | (SArea[PVarC(EI, king, opp)] & att);

						if (!(push &(~(King(opp) | Bishop(opp) | prp | double_att))))
						{
							EI.mul = 0;
							return;
						}
					}
				}
			}
		}
	}

	if (F(Major(me)))
	{
		if (T(Bishop(me)) && F(Knight(me)) && Single(Bishop(me)) && T(Pawn(me)))
		{
			int number = popcount<HPopCnt>(Pawn(me));

			if (number == 1)
			{
				if (Bishop(opp))
					EI.mul = MinF(EI.mul, kbpkbx<me>());

				else if (Knight(opp))
					EI.mul = MinF(EI.mul, kbpknx<me>());
			}

			else if (number == 2 && T(Bishop(opp)))
				EI.mul = MinF(EI.mul, kbppkbx<me>());
		}

		else if (!Bishop(me) && Knight(me) && Single(Knight(me)) && Pawn(me) && Single(Pawn(me)))
			EI.mul = MinF(EI.mul, knpkx<me>());
	}
	else if (F(Minor(me)))
	{
		if (F(Pawn(me)) && F(Rook(me)) && T(Queen(me)) && T(Pawn(opp)))
		{
			if (F(NonPawnKing(opp)) && Single(Pawn(opp)))
				EI.mul = MinF(EI.mul, kqkp<me>());

			else if (Rook(opp))
				EI.mul = MinF(EI.mul, kqkrpx<me>());
		}
		else if (F(Queen(me)) && T(Rook(me)) && Single(Rook(me)))
		{
			int number = popcount<HPopCnt>(Pawn(me));

			if (number <= 3)
			{
				if (number == 0)
				{
					if (Pawn(opp))
						EI.mul = MinF(EI.mul, krkpx<me>());
				}
				else if (Rook(opp))
				{
					if (number == 1)
					{
						int new_mul = krpkrx<me>();
						EI.mul = (new_mul <= 32 ? Min(EI.mul, new_mul) : new_mul);
					}
					else
					{
						if (number == 2)
							EI.mul = MinF(EI.mul, krppkrx<me>());

						if (Pawn(opp))
						{
							if (number == 2)
								EI.mul = MinF(EI.mul, krppkrpx<me>());

							else if (Multiple(Pawn(opp)))
								EI.mul = MinF(EI.mul, krpppkrppx<me>());
						}
					}
				}

				else if (number == 1 && Bishop(opp))
					EI.mul = MinF(EI.mul, krpkbx<me>());
			}
		}
	}

	else if (!Pawn(me) && Single(Rook(me)) && !Queen(me) && Single(Bishop(me)) && !Knight(me) && Rook(opp))
		EI.mul = MinF(EI.mul, krbkrx<me>());

	if (F(NonPawnKing(opp)) && Current->turn == opp && F(Current->att[me] & King(opp))
		&& !(SArea[PVarC(EI, king, opp)] & (~(Current->att[me] | Piece(opp)))) && F(Current->patt[opp] & Piece(me))
		&& F(Shift(opp, Pawn(opp)) & (~EI.occ)))
		EI.mul = 0;
}

template <int me> static int krbkrx()
{
	if (King(opp) & Interior)
		return 1;
	return 16;
}

template <int me> static int kpkx()
{
	uint64 u = 0;

	if (me == White)
		u = Kpk[Current->turn][lsb(Pawn(White))][lsb(King(White))] & Bit(lsb(King(Black)));
	else
		u = Kpk[Current->turn ^ 1][63 - lsb(Pawn(Black))][63 - lsb(King(Black))] & Bit(63 - lsb(King(White)));

	if (u)
		return 32;

	else if (Piece(opp) ^ King(opp))
		return 1;

	else
		return 0;
}

template <int me> static int knpkx()
{
	if (Pawn(me) & Line(me, 6) & (File[0] | File[7]))
	{
		int sq = lsb(Pawn(me));

		if (SArea[sq] & King(opp) & (Line(me, 6) | Line(me, 7)))
			return 0;

		if (Square(sq + Push(me)) == IKing(me) && (SArea[lsb(King(me))] && SArea[lsb(King(opp))] & Line(me, 7)))
			return 0;
	}
	else if (Pawn(me) & Line(me, 5) & (File[0] | File[7]))
	{
		int sq = lsb(Pawn(me));

		if (Square(sq + Push(me)) == IPawn(opp))
		{
			if (SArea[sq + Push(me)] & King(opp) & Line(me, 7))
				return 0;

			if ((SArea[sq + Push(me)] & SArea[lsb(King(opp))] & Line(me, 7))
				&& (!(NAtt[sq + Push(me)] & Knight(me)) || Current->turn == opp))
				return 0;
		}
	}

	return 32;
}

template <int me> static int krpkrx()
{
	int mul = 32;
	int sq = lsb(Pawn(me));
	int rrank = CRank(me, sq);
	int o_king = lsb(King(opp));
	int o_rook = lsb(Rook(opp));
	int m_king = lsb(King(me));
	int add_mat = T(Piece(opp) ^ King(opp) ^ Rook(opp));
	int clear = F(add_mat) || F((PWay[opp][sq]
		| PIsolated[File(sq)]) & Forward[opp][Rank(sq + Push(me))] & (Piece(opp) ^ King(opp) ^ Rook(opp)));

	if (!clear)
		return 32;

	if (!add_mat && !(Pawn(me) & (File[0] | File[7])))
	{
		int m_rook = lsb(Rook(me));

		if (CRank(me, o_king) < CRank(me, m_rook) && CRank(me, m_rook) < rrank && CRank(me, m_king) >= rrank - 1
			&& CRank(me, m_king) > CRank(me, m_rook) && ((SArea[m_king] & Pawn(me))
			|| (Current->turn == me && Abs(File(sq) - File(m_king)) <= 1 && Abs(rrank - CRank(me, m_king)) <= 2)))
			return 128;

		if (SArea[m_king] & Pawn(me))
		{
			if (rrank >= 4)
			{
				if ((File(sq) < File(m_rook) && File(m_rook) < File(o_king))
					|| (File(sq) > File(m_rook) && File(m_rook) > File(o_king)))
					return 128;
			}
			else if (rrank >= 2)
			{
				if (!(Pawn(me) & (File[1] | File[6])) && rrank + Abs(File(sq) - File(m_rook)) > 4
					&& ((File(sq) < File(m_rook) && File(m_rook) < File(o_king))
					|| (File(sq) > File(m_rook) && File(m_rook) > File(o_king))))
					return 128;
			}
		}
	}

	if (PWay[me][sq] & King(opp))
	{
		if (Pawn(me) & (File[0] | File[7]))
			mul = Min(mul, add_mat << 3);

		if (rrank <= 3)
			mul = Min(mul, add_mat << 3);

		if (rrank == 4 && CRank(me, m_king) <= 4 && CRank(me, o_rook) == 5 && T(King(opp) & (Line(me, 6) | Line(me, 7)))
			&& (Current->turn != me || F(PAtt[me][sq] & RookAttacks(lsb(Rook(me)), PieceAll) & (~SArea[o_king]))))
			mul = Min(mul, add_mat << 3);

		if (rrank >= 5 && CRank(me, o_rook) <= 1 && (Current->turn != me || Check(me) || Dist(m_king, sq) >= 2))
			mul = Min(mul, add_mat << 3);

		if (T(King(opp) & (File[1] | File[2] | File[6] | File[7])) && T(Rook(opp) & Line(me, 7))
			&& T(Between[o_king][o_rook] & (File[3] | File[4])) && F(Rook(me) & Line(me, 7)))
			mul = Min(mul, add_mat << 3);
		return mul;
	}
	else if (rrank == 6 && (Pawn(me) & (File[0] | File[7])) && ((PSupport[me][sq] | PWay[opp][sq]) & Rook(opp))
		&& CRank(me, o_king) >= 6)
	{
		int dist = Abs(File(sq) - File(o_king));

		if (dist <= 3)
			mul = Min(mul, add_mat << 3);

		if (dist == 4 && ((PSupport[me][o_king] & Rook(me)) || Current->turn == opp))
			mul = Min(mul, add_mat << 3);
	}

	if (SArea[o_king] & PWay[me][sq] & Line(me, 7))
	{
		if (rrank <= 4 && CRank(me, m_king) <= 4 && CRank(me, o_rook) == 5)
			mul = Min(mul, add_mat << 3);

		if (rrank == 5 && CRank(me, o_rook) <= 1 && Current->turn != me
			|| (F(SArea[m_king] & PAtt[me][sq] & (~SArea[o_king])) && (Check(me) || Dist(m_king, sq) >= 2)))
			mul = Min(mul, add_mat << 3);
	}

	if (T(PWay[me][sq] & Rook(me)) && T(PWay[opp][sq] & Rook(opp)))
	{
		if (King(opp) & (File[0] | File[1] | File[6] | File[7]) & Line(me, 6))
			mul = Min(mul, add_mat << 3);

		else if ((Pawn(me) & (File[0] | File[7])) && (King(opp) & (Line(me, 5) | Line(me, 6)))
			&& Abs(File(sq) - File(o_king)) <= 2 && File(sq) != File(o_king))
			mul = Min(mul, add_mat << 3);
	}

	if (Abs(File(sq) - File(o_king)) <= 1 && Abs(File(sq) - File(o_rook)) <= 1 && CRank(me, o_rook) > rrank
		&& CRank(me, o_king) > rrank)
		mul = Min(mul, (Pawn(me) & (File[3] | File[4])) ? 12 : 16);

	return mul;
}

template <int me> static int krpkbx()
{
	if (!(Pawn(me) & Line(me, 5)))
		return 32;
	int sq = lsb(Pawn(me));

	if (!(PWay[me][sq] & King(opp)))
		return 32;
	int diag_sq = NB(me, BMask[sq + Push(me)]);

	if (CRank(me, diag_sq) > 1)
		return 32;
	uint64 mdiag = FullLine[sq + Push(me)][diag_sq] | Bit(sq + Push(me)) | Bit(diag_sq);
	int check_sq = NB(me, BMask[sq - Push(me)]);
	uint64 cdiag = FullLine[sq - Push(me)][check_sq] | Bit(sq - Push(me)) | Bit(check_sq);

	if ((mdiag | cdiag) & (Piece(opp) ^ King(opp) ^ Bishop(opp)))
		return 32;

	if (cdiag & Bishop(opp))
		return 0;

	if ((mdiag & Bishop(opp)) && (Current->turn == opp || !(King(me) & PAtt[opp][sq + Push(me)])))
		return 0;
	return 32;
}

template <int me> static int kqkp()
{
	if (F(SArea[lsb(King(opp))] & Pawn(opp) & Line(me, 1) & (File[0] | File[2] | File[5] | File[7])))
		return 32;

	if (PWay[opp][lsb(Pawn(opp))] & (King(me) | Queen(me)))
		return 32;

	if (Pawn(opp) & (File[0] | File[7]))
		return 1;
	else
		return 4;
}

template <int me> static int kqkrpx()
{
	int rsq = lsb(Rook(opp));
	uint64 pawns = SArea[lsb(King(opp))] & PAtt[me][rsq] & Pawn(opp) & Interior & Line(me, 6);

	if (pawns && CRank(me, lsb(King(me))) <= 4)
		return 0;
	return 32;
}

template <int me> static int krkpx()
{
	if (T(SArea[lsb(King(opp))] & Pawn(opp) & Line(me, 1)) & F(PWay[opp][NB(me, Pawn(opp))] & King(me)))
		return 0;
	return 32;
}

template <int me> static int krppkrpx()
{
	if (Current->passer & Pawn(me))
	{
		if (Single(Current->passer & Pawn(me)))
		{
			int sq = lsb(Current->passer & Pawn(me));

			if (PWay[me][sq] & King(opp) & (File[0] | File[1] | File[6] | File[7]))
			{
				int opp_king = lsb(King(opp));

				if (SArea[opp_king] & Pawn(opp))
				{
					int king_file = File(opp_king);

					if (!((~(File[king_file] | PIsolated[king_file])) & Pawn(me)))
						return 1;
				}
			}
		}

		return 32;
	}

	if (F((~(PWay[opp][lsb(King(opp))] | PSupport[me][lsb(King(opp))])) & Pawn(me)))
		return 0;
	return 32;
}

template <int me> static int krpppkrppx()
{
	if (T(Current->passer & Pawn(me)) || F((SArea[lsb(Pawn(opp))] | SArea[msb(Pawn(opp))]) & Pawn(opp)))
		return 32;

	if (F((~(PWay[opp][lsb(King(opp))] | PSupport[me][lsb(King(opp))])) & Pawn(me)))
		return 0;
	return 32;
}

template <int me> static int kbpkbx()
{
	int sq = lsb(Pawn(me));
	uint64 u = 0;

	if ((T(Board->bb[ILight(me)]) && T(Board->bb[IDark(opp)]))
		|| (T(Board->bb[IDark(me)]) && T(Board->bb[ILight(opp)])))
	{
		if (CRank(me, sq) <= 4)
			return 0;

		if (T(PWay[me][sq] & King(opp)) && CRank(me, sq) <= 5)
			return 0;

		for (u = Bishop(opp); T(u); Cut(u))
		{
			if (CRank(me, lsb(u)) <= 4 && T(BishopAttacks(lsb(u), PieceAll) & PWay[me][sq]))
				return 0;

			if (Current->turn == opp && T(BishopAttacks(lsb(u), PieceAll) & Pawn(me)))
				return 0;
		}
	}

	else if (T(PWay[me][sq] & King(opp)) && T(King(opp) & LightArea) != T(Bishop(me) & LightArea))
		return 0;
	return 32;
}

template <int me> static int kbpknx()
{
	uint64 u = 0;

	if (T(PWay[me][lsb(Pawn(me))] & King(opp)) && T(King(opp) & LightArea) != T(Bishop(me) & LightArea))
		return 0;

	if (Current->turn == opp)
		for (u = Knight(opp); T(u); Cut(u))
			if (NAtt[lsb(u)] & Pawn(me))
				return 0;

	return 32;
}

template <int me> static int kbppkbx()
{
	int sq1 = NB(me, Pawn(me));
	int sq2 = NB(opp, Pawn(me));
	int o_king = lsb(King(opp));
	int o_bishop = lsb(Bishop(opp));

	if (File(sq1) == File(sq2))
	{
		if (CRank(me, sq2) <= 3)
			return 0;

		if (T(PWay[me][sq2] & King(opp)) && CRank(me, sq2) <= 5)
			return 0;
	}
	else if (PIsolated[File(sq1)] & Pawn(me))
	{
		if (T(King(opp) & LightArea) != T(Bishop(me) & LightArea))
		{
			if (T((SArea[o_king] | King(opp)) & Bit(sq2 + Push(me)))
				&& T(BishopAttacks(o_bishop, PieceAll) & Bit(sq2 + Push(me))))
				if (T((SArea[o_king] | King(opp)) & Bit((sq2 & 0xFFFFFFF8) | File(sq1)))
					&& T(BishopAttacks(o_bishop, PieceAll) & Bit((sq2 & 0xFFFFFFF8) | File(sq1))))
					return 0;
		}
	}
	return 32;
}

template <int me> static int krppkrx()
{
	int sq1 = NB(me, Pawn(me));
	int sq2 = NB(opp, Pawn(me));

	if ((Piece(opp) ^ King(opp) ^ Rook(opp)) & Forward[me][Rank(sq1 - Push(me))])
		return 32;

	if (File(sq1) == File(sq2))
	{
		if (T(PWay[me][sq2] & King(opp)))
			return 16;
		return 32;
	}

	if (T(PIsolated[File(sq2)] & Pawn(me)) && T((File[0] | File[7]) & Pawn(me)) && T(King(opp) & Shift(me, Pawn(me))))
	{
		if (CRank(me, sq2) == 5 && CRank(me, sq1) == 4 && T(Rook(opp) & (Line(me, 5) | Line(me, 6))))
			return 10;

		else if (CRank(me, sq2) < 5)
			return 16;
	}
	return 32;
}

#endif