#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

template <int me> void gen_root_moves()
{
	int i = 0;
	int depth = -256;
	int * p = NULL;
	int move = 0;
	int killer = 0;

	GEntry* Entry;
	GPVEntry* PVEntry;

	if (Entry = probe_hash())
	{
		if (T(Entry->move) && Entry->low_depth > depth)
		{
			depth = Entry->low_depth;
			killer = Entry->move;
		}
	}

	if (PVEntry = probe_pv_hash())
	{
		if (PVEntry->depth > depth && T(PVEntry->move))
		{
			depth = PVEntry->depth;
			killer = PVEntry->move;
		}
	}

	Current->killer[0] = killer;

	if (Check(me))
		Current->stage = stage_evasion;
	else
	{
		Current->stage = stage_search;
		Current->ref[0] = RefM(Current->move).ref[0];
		Current->ref[1] = RefM(Current->move).ref[1];
	}
	Current->gen_flags = 0;
	p = RootList;
	Current->current = Current->moves;
	Current->moves[0] = 0;

	while (move = get_move<me, 0>())
	{
		if (IsIllegal(me, move))
			continue;

		if (p > RootList && move == killer)
			continue;

		if (SearchMoves)
		{
			for (i = 0; i < SMPointer; i++)
				if (SMoves[i] == move)
					goto keep_move;
			continue;
		}
	keep_move:
		*p = move;
		p++;
	}
	*p = 0;
}

template <int me, bool up> static int* gen_captures(int* list)
{
	uint64 u = 0, v = 0;

	if (Current->ep_square)
		for (v = PAtt[opp][Current->ep_square] & Pawn(me); T(v); Cut(v))
			AddMove(lsb(v), Current->ep_square, FlagEP, MvvLva[IPawn(me)][IPawn(opp)])

			for (u = Pawn(me) & Line(me, 6); T(u); Cut(u))
				if (F(Square(lsb(u) + Push(me))))
				{
					AddMove(lsb(u), lsb(u) + Push(me), FlagPQueen, MvvLvaPromotion)

						if (up || (NAtt[lsb(King(opp))] & Bit(lsb(u) + Push(me))))
							AddMove(lsb(u), lsb(u) + Push(me), FlagPKnight, MvvLvaPromotionKnight)

							if (up)
							{
								AddMove(lsb(u), lsb(u) + Push(me), FlagPRook, MvvLvaPromotionKnight)
									AddMove(lsb(u), lsb(u) + Push(me), (Bit(lsb(u)) & LightArea) ? FlagPDark : FlagPLight,
									MvvLvaPromotionKnight)
							}
				}

	for (v = ShiftW(opp, Current->mask) & Pawn(me) & Line(me, 6); T(v); Cut(v))
	{
		AddMove(lsb(v), lsb(v) + PushE(me), FlagPQueen, MvvLvaPromotionCap(Square(lsb(v) + PushE(me))))

			if (up || (NAtt[lsb(King(opp))] & Bit(lsb(v) + PushE(me))))
				AddMove(lsb(v), lsb(v) + PushE(me), FlagPKnight, MvvLvaPromotionKnightCap(Square(lsb(v) + PushE(me))))

				if (up)
				{
					AddMove(lsb(v), lsb(v) + PushE(me), FlagPRook, MvvLvaPromotionKnightCap(Square(lsb(v) + PushE(me))))
						AddMove(lsb(v), lsb(v) + PushE(me), (Bit(lsb(v)) & LightArea) ? FlagPDark : FlagPLight,
						MvvLvaPromotionKnightCap(Square(lsb(v) + PushE(me))))
				}
	}

	for (v = ShiftE(opp, Current->mask) & Pawn(me) & Line(me, 6); T(v); Cut(v))
	{
		AddMove(lsb(v), lsb(v) + PushW(me), FlagPQueen, MvvLvaPromotionCap(Square(lsb(v) + PushW(me))))

			if (up || (NAtt[lsb(King(opp))] & Bit(lsb(v) + PushW(me))))
				AddMove(lsb(v), lsb(v) + PushW(me), FlagPKnight, MvvLvaPromotionKnightCap(Square(lsb(v) + PushW(me))))

				if (up)
				{
					AddMove(lsb(v), lsb(v) + PushW(me), FlagPRook, MvvLvaPromotionKnightCap(Square(lsb(v) + PushW(me))))
						AddMove(lsb(v), lsb(v) + PushW(me), (Bit(lsb(v)) & LightArea) ? FlagPDark : FlagPLight,
						MvvLvaPromotionKnightCap(Square(lsb(v) + PushW(me))))
				}
	}

	if (F(Current->att[me] & Current->mask))
		goto finish;

	for (v = ShiftW(opp, Current->mask) & Pawn(me) & (~Line(me, 6)); T(v); Cut(v))
		AddCaptureP(IPawn(me), lsb(v), lsb(v) + PushE(me), 0)

		for (v = ShiftE(opp, Current->mask) & Pawn(me) & (~Line(me, 6)); T(v); Cut(v))
			AddCaptureP(IPawn(me), lsb(v), lsb(v) + PushW(me), 0)

			for (v = SArea[lsb(King(me))] & Current->mask &(~Current->att[opp]); T(v); Cut(v))
				AddCaptureP(IKing(me), lsb(King(me)), lsb(v), 0)

				for (u = Knight(me); T(u); Cut(u))
					for (v = NAtt[lsb(u)] & Current->mask; T(v); Cut(v))
						AddCaptureP(IKnight(me), lsb(u), lsb(v), 0)

						for (u = Bishop(me); T(u); Cut(u))
							for (v = BishopAttacks(lsb(u), PieceAll) & Current->mask; T(v); Cut(v))
								AddCapture(lsb(u), lsb(v), 0)

								for (u = Rook(me); T(u); Cut(u))
									for (v = RookAttacks(lsb(u), PieceAll) & Current->mask; T(v); Cut(v))
										AddCaptureP(IRook(me), lsb(u), lsb(v), 0)

										for (u = Queen(me); T(u); Cut(u))
											for (v = QueenAttacks(lsb(u), PieceAll) & Current->mask; T(v); Cut(v))
												AddCaptureP(IQueen(me), lsb(u), lsb(v), 0)

											finish:
	*list = 0;
	return list;
}

template <int me> static int* gen_evasions(int* list)
{
	int king = 0, att_sq = 0, from = 0;
	uint64 att = 0, esc = 0, b = 0, u = 0;

	king = lsb(King(me));
	att = (NAtt[king] & Knight(opp)) | (PAtt[me][king] & Pawn(opp));

	for (u = (BMask[king] & BSlider(opp)) | (RMask[king] & RSlider(opp)); T(u); u ^= b)
	{
		b = Bit(lsb(u));

		if (F(Between[king][lsb(u)] & PieceAll))
			att |= b;
	}
	att_sq = lsb(att);
	esc = SArea[king] & (~(Piece(me) | Current->att[opp])) & Current->mask;

	if (Square(att_sq) >= WhiteLight)
		esc &= ~FullLine[king][att_sq];
	Cut(att);

	if (att)
	{
		att_sq = lsb(att);

		if (Square(att_sq) >= WhiteLight)
			esc &= ~FullLine[king][att_sq];

		for (; T(esc); Cut(esc))
			AddCaptureP(IKing(me), king, lsb(esc), 0) * list = 0;
		return list;
	}

	if (Bit(att_sq) & Current->mask)
	{
		if (T(Current->ep_square) && Current->ep_square == att_sq + Push(me))
			for (u = PAtt[opp][att_sq + Push(me)] & Pawn(me); T(u); Cut(u))
				AddMove(lsb(u), att_sq + Push(me), FlagEP, MvvLva[IPawn(me)][IPawn(opp)])
	}

	for (u = PAtt[opp][att_sq] & Pawn(me); T(u); Cut(u))
	{
		from = lsb(u);

		if (Bit(att_sq) & Line(me, 7))
			AddMove(from, att_sq, FlagPQueen, MvvLvaPromotionCap(Square(att_sq)))

		else if (Bit(att_sq) & Current->mask)
		AddCaptureP(IPawn(me), from, att_sq, 0)
	}

	for (; T(esc); Cut(esc))
		AddCaptureP(IKing(me), king, lsb(esc), 0)att = Between[king][att_sq];

	for (u = Shift(opp, att) & Pawn(me); T(u); Cut(u))
	{
		from = lsb(u);

		if (Bit(from) & Line(me, 6))
			AddMove(from, from + Push(me), FlagPQueen, MvvLvaPromotion)

		else if (F(~Current->mask))
		AddMove(from, from + Push(me), 0, 0)
	}

	if (F(~Current->mask))
	{
		for (u = Shift(opp, Shift(opp, att)) & Line(me, 1) & Pawn(me); T(u); Cut(u))
			if (F(Square(lsb(u) + Push(me))))
				AddMove(lsb(u), lsb(u) + 2 * Push(me), 0, 0)
	}

	att |= Bit(att_sq);

	for (u = Knight(me); T(u); Cut(u))
		for (esc = NAtt[lsb(u)] & att; T(esc); esc ^= b)
		{
			b = Bit(lsb(esc));

			if (b & Current->mask)
				AddCaptureP(IKnight(me), lsb(u), lsb(esc), 0)
		}

	for (u = Bishop(me); T(u); Cut(u))
		for (esc = BishopAttacks(lsb(u), PieceAll) & att; T(esc); esc ^= b)
		{
			b = Bit(lsb(esc));

			if (b & Current->mask)
				AddCapture(lsb(u), lsb(esc), 0)
		}

	for (u = Rook(me); T(u); Cut(u))
		for (esc = RookAttacks(lsb(u), PieceAll) & att; T(esc); esc ^= b)
		{
			b = Bit(lsb(esc));

			if (b & Current->mask)
				AddCaptureP(IRook(me), lsb(u), lsb(esc), 0)
		}

	for (u = Queen(me); T(u); Cut(u))
		for (esc = QueenAttacks(lsb(u), PieceAll) & att; T(esc); esc ^= b)
		{
			b = Bit(lsb(esc));

			if (b & Current->mask)
				AddCaptureP(IQueen(me), lsb(u), lsb(esc), 0)
		}

	*list = 0;
	return list;
}

static void mark_evasions(int* list)
{
	for (; T(*list); list++)
	{
		register int move = (*list) & 0xFFFF;

		if (F(Square(To(move))) && F(move & 0xE000))
		{
			if (move == Current->ref[0])
				*list |= RefOneScore;

			else if (move == Current->ref[1])
				* list |= RefTwoScore;

			else if (move == Current->killer[1])
				* list |= KillerOneScore;

			else if (move == Current->killer[2])
				* list |= KillerTwoScore;

			else
				* list |= HistoryP(Square(From(move)), From(move), To(move));
		}
	}
}

template <int me> static int* gen_quiet_moves(int* list)
{
	int to = 0;
	uint64 u = 0, v = 0;
	uint64 occ = PieceAll;
	uint64 free = ~occ;

	if (me == White)
	{
		if (T(Current->castle_flags & CanCastle_OO) && F(occ & 0x60) && F(Current->att[Black] & 0x70))
			AddHistoryP(IKing(White), 4, 6, FlagCastling)

			if (T(Current->castle_flags & CanCastle_OOO) && F(occ & 0xE) && F(Current->att[Black] & 0x1C))
				AddHistoryP(IKing(White), 4, 2, FlagCastling)
	}
	else
	{
		if (T(Current->castle_flags & CanCastle_oo) && F(occ & 0x6000000000000000)
			&& F(Current->att[White] & 0x7000000000000000))
			AddHistoryP(IKing(Black), 60, 62, FlagCastling)

			if (T(Current->castle_flags & CanCastle_ooo) && F(occ & 0x0E00000000000000)
				&& F(Current->att[White] & 0x1C00000000000000))
				AddHistoryP(IKing(Black), 60, 58, FlagCastling)
	}

	for (v = Shift(me, Pawn(me)) & free &(~Line(me, 7)); T(v); Cut(v))
	{
		to = lsb(v);

		if (T(Bit(to) & Line(me, 2)) && F(Square(to + Push(me))))
			AddHistoryP(IPawn(me), to - Push(me), to + Push(me), 0)

			AddHistoryP(IPawn(me), to - Push(me), to, 0)
	}

	for (u = Knight(me); T(u); Cut(u))
		for (v = free & NAtt[lsb(u)]; T(v); Cut(v))
			AddHistoryP(IKnight(me), lsb(u), lsb(v), 0)

			for (u = Bishop(me); T(u); Cut(u))
				for (v = free & BishopAttacks(lsb(u), occ); T(v); Cut(v))
					AddHistory(lsb(u), lsb(v))

					for (u = Rook(me); T(u); Cut(u))
						for (v = free & RookAttacks(lsb(u), occ); T(v); Cut(v))
							AddHistoryP(IRook(me), lsb(u), lsb(v), 0)

							for (u = Queen(me); T(u); Cut(u))
								for (v = free & QueenAttacks(lsb(u), occ); T(v); Cut(v))
									AddHistoryP(IQueen(me), lsb(u), lsb(v), 0)

									for (v = SArea[lsb(King(me))] & free &(~Current->att[opp]); T(v); Cut(v))
										AddHistoryP(IKing(me), lsb(King(me)), lsb(v), 0) * list = 0;
	return list;
}

template <int me> static int* gen_checks(int* list)
{
	int from = 0;
	int king = lsb(King(opp));
	uint64 u = 0, v = 0, target = 0, b_target = 0, r_target = 0, xray = 0;
	uint64 clear = ~(Piece(me) | Current->mask);

	for (u = Current->xray[me] & Piece(me); T(u); Cut(u))
	{
		from = lsb(u);
		target = clear &(~FullLine[king][from]);

		if (Square(from) == IPawn(me))
		{
			if (F(Bit(from + Push(me)) & Line(me, 7)))
			{
				if (T(Bit(from + Push(me)) & target) && F(Square(from + Push(me))))
					AddMove(from, from + Push(me), 0, MvvLvaXray)

					for (v = PAtt[me][from] & target & Piece(opp); T(v); Cut(v))
						AddMove(from, lsb(v), 0, MvvLvaXrayCap(Square(lsb(v))))
			}
		}
		else
		{
			if (Square(from) < WhiteLight)
				v = NAtt[from] & target;

			else if (Square(from) < WhiteRook)
				v = BishopAttacks(from, PieceAll) & target;

			else if (Square(from) < WhiteQueen)
				v = RookAttacks(from, PieceAll) & target;

			else if (Square(from) < WhiteKing)
				v = QueenAttacks(from, PieceAll) & target;

			else
				v = SArea[from] & target &(~Current->att[opp]);

			for (; T(v); Cut(v))
				AddMove(from, lsb(v), 0, MvvLvaXrayCap(Square(lsb(v))))
		}
	}
	xray = ~(Current->xray[me] & Board->bb[me]);

	for (u = Knight(me) & NArea[king] & xray; T(u); Cut(u))
		for (v = NAtt[king] & NAtt[lsb(u)] & clear; T(v); Cut(v))
			AddCaptureP(IKnight(me), lsb(u), lsb(v), 0)

			for (u = DArea[king] & Pawn(me) & (~Line(me, 6)) & xray; T(u); Cut(u))
			{
				from = lsb(u);

				for (v = PAtt[me][from] & PAtt[opp][king] & clear & Piece(opp); T(v); Cut(v))
					AddCaptureP(IPawn(me), from, lsb(v), 0)

					if (F(Square(from + Push(me))) && T(Bit(from + Push(me)) & PAtt[opp][king]))
						AddMove(from, from + Push(me), 0, 0)
			}

	b_target = BishopAttacks(king, PieceAll) & clear;
	r_target = RookAttacks(king, PieceAll) & clear;

	for (u = (Odd(king ^ Rank(king)) ? Board->bb[WhiteLight | me] : Board->bb[WhiteDark | me]) & xray; T(u); Cut(u))
		for (v = BishopAttacks(lsb(u), PieceAll) & b_target; T(v); Cut(v))
			AddCapture(lsb(u), lsb(v), 0)

			for (u = Rook(me) & xray; T(u); Cut(u))
				for (v = RookAttacks(lsb(u), PieceAll) & r_target; T(v); Cut(v))
					AddCaptureP(IRook(me), lsb(u), lsb(v), 0)

					for (u = Queen(me) & xray; T(u); Cut(u))
						for (v = QueenAttacks(lsb(u), PieceAll) & (b_target | r_target); T(v); Cut(v))
							AddCaptureP(IQueen(me), lsb(u), lsb(v), 0)
							* list = 0;
	return list;
}

template <int me> static int* gen_delta_moves(int* list)
{
	int to = 0;
	uint64 u = 0, v = 0;
	uint64 occ = PieceAll;
	uint64 free = ~occ;

	if (me == White)
	{
		if (T(Current->castle_flags & CanCastle_OO) && F(occ & 0x60) && F(Current->att[Black] & 0x70))
			AddCDeltaP(IKing(White), 4, 6, FlagCastling)

			if (T(Current->castle_flags & CanCastle_OOO) && F(occ & 0xE) && F(Current->att[Black] & 0x1C))
				AddCDeltaP(IKing(White), 4, 2, FlagCastling)
	}
	else
	{
		if (T(Current->castle_flags & CanCastle_oo) && F(occ & 0x6000000000000000)
			&& F(Current->att[White] & 0x7000000000000000))
			AddCDeltaP(IKing(Black), 60, 62, FlagCastling)

			if (T(Current->castle_flags & CanCastle_ooo) && F(occ & 0x0E00000000000000)
				&& F(Current->att[White] & 0x1C00000000000000))
				AddCDeltaP(IKing(Black), 60, 58, FlagCastling)
	}

	for (v = Shift(me, Pawn(me)) & free &(~Line(me, 7)); T(v); Cut(v))
	{
		to = lsb(v);

		if (T(Bit(to) & Line(me, 2)) && F(Square(to + Push(me))))
			AddCDeltaP(IPawn(me), to - Push(me), to + Push(me), 0)

			AddCDeltaP(IPawn(me), to - Push(me), to, 0)
	}

	for (u = Knight(me); T(u); Cut(u))
		for (v = free & NAtt[lsb(u)]; T(v); Cut(v))
			AddCDeltaP(IKnight(me), lsb(u), lsb(v), 0)

			for (u = Bishop(me); T(u); Cut(u))
				for (v = free & BishopAttacks(lsb(u), occ); T(v); Cut(v))
					AddCDelta(lsb(u), lsb(v))

					for (u = Rook(me); T(u); Cut(u))
						for (v = free & RookAttacks(lsb(u), occ); T(v); Cut(v))
							AddCDeltaP(IRook(me), lsb(u), lsb(v), 0)

							for (u = Queen(me); T(u); Cut(u))
								for (v = free & QueenAttacks(lsb(u), occ); T(v); Cut(v))
									AddCDeltaP(IQueen(me), lsb(u), lsb(v), 0)

									for (v = SArea[lsb(King(me))] & free &(~Current->att[opp]); T(v); Cut(v))
										AddCDeltaP(IKing(me), lsb(King(me)), lsb(v), 0) * list = 0;
	return list;
}

#endif