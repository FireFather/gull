#ifndef EVAL_H_INCLUDED
#define EVAL_H_INCLUDED

int Mobility[4][32];
int PasserGeneral[8];
int PasserBlocked[8];
int PasserFree[8];
int PasserSupported[8];
int PasserProtected[8];
int PasserConnected[8];
int PasserOutside[8];
int PasserCandidate[8];
int PasserClear[8];

sint16 Shelter[3][8];
sint16 StormBlocked[4];
sint16 StormShelterAtt[4];
sint16 StormConnected[4];
sint16 StormOpen[4];
sint16 StormFree[4];
sint16 PasserAtt[8];
sint16 PasserDef[8];
sint16 PasserAttLog[8];
sint16 PasserDefLog[8];

uint8 LogDist[16];

__forceinline void evaluate()
{
	HardwarePopCnt ? evaluation<1>() : evaluation<0>();
}

template <bool HPopCnt> static void evaluation()
{
	GEvalInfo EI;

	if (Current->eval_key == Current->key)
		return;

	Current->eval_key = Current->key;

	EI.king_w = lsb(King(White));
	EI.king_b = lsb(King(Black));
	EI.occ = PieceAll;
	Current->patt[White] = ShiftW(White, Pawn(White)) | ShiftE(White, Pawn(White));
	Current->patt[Black] = ShiftW(Black, Pawn(Black)) | ShiftE(Black, Pawn(Black));
	EI.area_w = (SArea[EI.king_w] | King(White)) & ((~Current->patt[White]) | Current->patt[Black]);
	EI.area_b = (SArea[EI.king_b] | King(Black)) & ((~Current->patt[Black]) | Current->patt[White]);
	Current->att[White] = Current->patt[White];
	Current->att[Black] = Current->patt[Black];
	Current->passer = 0;
	Current->threat = (Current->patt[White] & NonPawn(Black)) | (Current->patt[Black] & NonPawn(White));
	EI.score = Current->pst;

#define me White

	Current->xray[me] = 0;
	PVarC(EI, free, me) = Queen(opp) | King(opp) | (~(Current->patt[opp] | Pawn(me) | King(me)));
	DecV(EI.score, popcount<HPopCnt>(Shift(opp, EI.occ) & Pawn(me)) * PawnBlocked);

	if (Current->patt[me] & PVarC(EI, area, opp))
		PVarC(EI, king_att, me) = KingAttack;
	else
		PVarC(EI, king_att, me) = 0;
	eval_queens<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Rook(opp);
	eval_rooks<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Minor(opp);
	eval_bishops<me, HPopCnt>(EI);
	eval_knights<me, HPopCnt>(EI);

#undef me
#define me Black

	Current->xray[me] = 0;
	PVarC(EI, free, me) = Queen(opp) | King(opp) | (~(Current->patt[opp] | Pawn(me) | King(me)));
	DecV(EI.score, popcount<HPopCnt>(Shift(opp, EI.occ) & Pawn(me)) * PawnBlocked);

	if (Current->patt[me] & PVarC(EI, area, opp))
		PVarC(EI, king_att, me) = KingAttack;
	else
		PVarC(EI, king_att, me) = 0;
	eval_queens<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Rook(opp);
	eval_rooks<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Minor(opp);
	eval_bishops<me, HPopCnt>(EI);
	eval_knights<me, HPopCnt>(EI);

#undef me

	EI.PawnEntry = PawnHash + (Current->pawn_key & pawn_hash_mask);

	if (Current->pawn_key != EI.PawnEntry->key)
		eval_pawn_structure<HPopCnt>(EI.PawnEntry);
	EI.score += EI.PawnEntry->score;

	eval_king<White, HPopCnt>(EI);
	eval_king<Black, HPopCnt>(EI);
	Current->att[White] |= SArea[EI.king_w];
	Current->att[Black] |= SArea[EI.king_b];

	eval_passer<White, HPopCnt>(EI);
	eval_pieces<White, HPopCnt>(EI);
	eval_passer<Black, HPopCnt>(EI);
	eval_pieces<Black, HPopCnt>(EI);

	if (Current->material & FlagUnusualMaterial)
	{
		eval_unusual_material<HPopCnt>(EI);
		return;
	}
	EI.material = &Material[Current->material];
	Current->score = EI.material->score
		+ (((Opening(EI.score) * EI.material->phase) + (Endgame(EI.score) * (128 - (int)EI.material->phase))) >> 7);

	if (Current->ply >= 50)
		Current->score /= 2;

	if (Current->score > 0)
	{
		EI.mul = EI.material->mul[White];

		if (EI.material->flags & FlagCallEvalEndgame_w)
			eval_endgame<White, HPopCnt>(EI);
		Current->score -= (Min(Current->score, 100) * (int)EI.PawnEntry->draw[White]) >> 6;
	}
	else if (Current->score < 0)
	{
		EI.mul = EI.material->mul[Black];

		if (EI.material->flags & FlagCallEvalEndgame_b)
			eval_endgame<Black, HPopCnt>(EI);
		Current->score += (Min(-Current->score, 100) * (int)EI.PawnEntry->draw[Black]) >> 6;
	}
	else
		EI.mul = Min(EI.material->mul[White], EI.material->mul[Black]);
	Current->score = (Current->score * EI.mul) >> 5;

	if (Current->turn)
		Current->score = -Current->score;
	UpdateDelta
}

template <int me, bool HPopCnt> static __forceinline void eval_pawns(GPawnEntry* PawnEntry, GPawnEvalInfo &PEI)
{
	int kf = File(PVarC(PEI, king, me));
	int kr = Rank(PVarC(PEI, king, me));
	int start = 0, inc = 0;

	if (kf <= 3)
	{
		start = Max(kf - 1, 0);
		inc = 1;
	}
	else
	{
		start = Min(kf + 1, 7);
		inc = -1;
	}
	int shelter = 0;
	uint64 mpawns = Pawn(me) & Forward[me][me ? Min(kr + 1, 7) : Max(kr - 1, 0)];

	for (int file = start, i = 0; i < 3; file += inc, i++)
	{
		shelter += Shelter[i][CRank(me, NBZ(me, mpawns & File[file]))];
		int rank = 0;

		if (Pawn(opp) & File[file])
		{
			int sq = NB(me, Pawn(opp) & File[file]);

			if ((rank = CRank(opp, sq)) < 6)
			{
				if (rank >= 3)
					shelter += StormBlocked[rank - 3];

				if (uint64 u = (PIsolated[File(sq)] & Forward[opp][Rank(sq)] & Pawn(me)))
				{
					int square = NB(opp, u);
					uint64 att_sq = PAtt[me][square] & PWay[opp][sq];

					if ((File[File(square)] | PIsolated[File(square)]) & King(me))
						if (!(PVarC(PEI, double_att, me) & att_sq) || (Current->patt[opp] & att_sq))
						{
							if (PWay[opp][square] & Pawn(me))
								continue;

							if (!(PawnAll & PWay[opp][sq] & Forward[me][Rank(square)]))
							{
								if (rank >= 3)
								{
									shelter += StormShelterAtt[rank - 3];

									if (PVarC(PEI, patt, opp) & Bit(sq + Push(opp)))
										shelter += StormConnected[rank - 3];

									if (!(PWay[opp][sq] & PawnAll))
										shelter += StormOpen[rank - 3];
								}

								if (!((File[File(sq)] | PIsolated[File(sq)]) & King(opp)) && rank <= 4)
									shelter += StormFree[rank - 1];
							}
						}
				}
			}
		}
		else
		{
			if (!(Pawn(me) & File[file]))
				shelter += StormOfValue;
		}
	}
	PawnEntry->shelter[me] = shelter;

	uint64 b = 0;
	int min_file = 7, max_file = 0;

	for (uint64 u = Pawn(me); T(u); u ^= b)
	{
		int sq = lsb(u);
		b = Bit(sq);
		int rank = Rank(sq);
		int rrank = CRank(me, sq);
		int file = File(sq);
		uint64 way = PWay[me][sq];
		int next = Square(sq + Push(me));

		if (file < min_file)
			min_file = file;

		if (file > max_file)
			max_file = file;

		int isolated = !(Pawn(me) & PIsolated[file]);
		int doubled = T(Pawn(me) & (File[file] ^ b));
		int open = !(PawnAll & way);
		int up = !(PVarC(PEI, patt, me) & b);

		if (isolated)
		{
			if (open)
				DecV(PEI.score, IsolatedOpen);
			else
			{
				DecV(PEI.score, IsolatedClosed);

				if (next == IPawn(opp))
					DecV(PEI.score, IsolatedBlocked);
			}

			if (doubled)
			{
				if (open)
					DecV(PEI.score, IsolatedDoubledOpen);
				else
					DecV(PEI.score, IsolatedDoubledClosed);
			}
		}
		else
		{
			if (doubled)
			{
				if (open)
					DecV(PEI.score, DoubledOpen);
				else
					DecV(PEI.score, DoubledClosed);
			}

			if (rrank >= 3 && (b &(File[2] | File[3] | File[4] | File[5])) && next != IPawn(opp)
				&& (PIsolated[file] & Line[rank] & Pawn(me)))
				IncV(PEI.score, PawnChainLinear * (rrank - 3) + PawnChain);
		}
		int backward = 0;

		if (!(PSupport[me][sq] & Pawn(me)))
		{
			if (isolated)
				backward = 1;

			else if (uint64 v = (PawnAll | PVarC(PEI, patt, opp)) & way)
				if (IsGreater(me, NB(me, PVarC(PEI, patt, me) & way), NB(me, v)))
					backward = 1;
		}

		if (backward)
		{
			if (open)
				DecV(PEI.score, BackwardOpen);
			else
				DecV(PEI.score, BackwardClosed);
		}

		else if (open)
			if (!(Pawn(opp) & PIsolated[file])
				|| popcount<HPopCnt>(Pawn(me) & PIsolated[file]) >= popcount<HPopCnt>(Pawn(opp) & PIsolated[file]))
				IncV(PEI.score, PasserCandidate[rrank]);

		if (up && next == IPawn(opp))
		{
			DecV(PEI.score, UpBlocked);

			if (backward)
			{
				if (rrank <= 2)
				{
					DecV(PEI.score, PasserTarget);

					if (rrank <= 1)
						DecV(PEI.score, PasserTarget);
				}

				for (uint64 v = PAtt[me][sq] & Pawn(me); v; Cut(v))
					if ((PSupport[me][lsb(v)] & Pawn(me)) == b)
					{
						DecV(PEI.score, ChainRoot);
						break;
					}
			}
		}

		if (open && !(PIsolated[file] & Forward[me][rank] & Pawn(opp)))
		{
			PawnEntry->passer[me] |= (uint8)(1 << file);

			if (rrank <= 2)
				continue;

			IncV(PEI.score, PasserGeneral[rrank]);
			int dist_att = Dist(PVarC(PEI, king, opp), sq + Push(me));
			int dist_def = Dist(PVarC(PEI, king, me), sq + Push(me));
			IncV(PEI.score,
				Compose256(0, dist_att * (int)PasserAtt[rrank] + LogDist[dist_att] * (int)PasserAttLog[rrank] - dist_def
				* (int)PasserDef[rrank] - (int)LogDist[dist_def] * (int)PasserDefLog[rrank]));

			if (PVarC(PEI, patt, me) & b)
				IncV(PEI.score, PasserProtected[rrank]);

			if (!(Pawn(opp) & West[file]) || !(Pawn(opp) & East[file]))
				IncV(PEI.score, PasserOutside[rrank]);
		}
	}
	uint64 files = 0;

	for (int i = 1; i < 7; i++)
		files |= (Pawn(me) >> (i << 3)) & 0xFF;
	int file_span = (files ? (msb(files) - lsb(files)) : 0);
	IncV(PEI.score, PawnFileSpan * file_span);
	PawnEntry->draw[me] = (7 - file_span) * Max(5 - popcount<HPopCnt>(files), 0);
}

template <bool HPopCnt> static void eval_pawn_structure(GPawnEntry* PawnEntry)
{
	GPawnEvalInfo PEI;

	for (int i = 0; i < sizeof(GPawnEntry) / sizeof(int); i++)
		*(((int*)PawnEntry) + i) = 0;

	PawnEntry->key = Current->pawn_key;

	PEI.patt_w = ShiftW(White, Pawn(White)) | ShiftE(White, Pawn(White));
	PEI.patt_b = ShiftW(Black, Pawn(Black)) | ShiftE(Black, Pawn(Black));
	PEI.double_att_w = ShiftW(White, Pawn(White)) & ShiftE(White, Pawn(White));
	PEI.double_att_b = ShiftW(Black, Pawn(Black)) & ShiftE(Black, Pawn(Black));
	PEI.king_w = lsb(King(White));
	PEI.king_b = lsb(King(Black));
	PEI.score = 0;

	eval_pawns<White, HPopCnt>(PawnEntry, PEI);
	eval_pawns<Black, HPopCnt>(PawnEntry, PEI);

	PawnEntry->score = PEI.score;
}

template <int me, bool HPopCnt> static __forceinline void eval_queens(GEvalInfo &EI)
{
	uint64 u = 0, b = 0;

	for (u = Queen(me); T(u); u ^= b)
	{
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = QueenAttacks(sq, EI.occ);
		Current->att[me] |= att;

		if (QMask[sq] & King(opp))
			if (uint64 v = Between[PVarC(EI, king, opp)][sq] & EI.occ)
				if (Single(v))
				{
					Current->xray[me] |= v;
					uint64 square = lsb(v);
					int piece = Square(square);
					int katt = 0;

					if (piece == IPawn(me))
					{
						if (!Square(square + Push(me)))
							IncV(EI.score, SelfPawnPin);
					}
					else if ((piece & 1) == me)
					{
						IncV(EI.score, SelfPiecePin);
						katt = 1;
					}
					else if (piece != IPawn(opp)
						&& !(((BMask[sq] & Bishop(opp)) | (RMask[sq] & Rook(opp)) | Queen(opp)) & v))
					{
						IncV(EI.score, WeakPin);

						if (!(Current->patt[opp] & v))
							katt = 1;
					}

					if (katt && !(att & PVarC(EI, area, opp)))
						PVarC(EI, king_att, me) += KingAttack;
				}

				else if (v == (v & Minor(opp)))
					IncV(EI.score, QKingRay);

		if (att & PVarC(EI, area, opp))
		{
			PVarC(EI, king_att, me) += KingQAttack;

			for (uint64 v = att & PVarC(EI, area, opp); T(v); Cut(v))
				if (FullLine[sq][lsb(v)] & att &((Rook(me) & RMask[sq]) | (Bishop(me) & BMask[sq])))
					PVarC(EI, king_att, me)++;
		}

		IncV(EI.score, Mobility[PieceType[WhiteQueen] - 1][popcount<HPopCnt>(att & PVarC(EI, free, me))]);

		if (att & PVarC(EI, free, me) & Pawn(opp))
			IncV(EI.score, TacticalMajorPawn);

		if (att & PVarC(EI, free, me) & Minor(opp))
			IncV(EI.score, TacticalMajorMinor);

		if (att & PVarC(EI, area, me))
			IncV(EI.score, KingDefQueen);
	}
}

template <int me, bool HPopCnt> static __forceinline void eval_rooks(GEvalInfo &EI)
{
	uint64 u = 0, b = 0;

	for (u = Rook(me); T(u); u ^= b)
	{
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = RookAttacks(sq, EI.occ);
		Current->att[me] |= att;

		if (RMask[sq] & King(opp))
			if (uint64 v = Between[PVarC(EI, king, opp)][sq] & EI.occ)
				if (Single(v))
				{
					Current->xray[me] |= v;
					uint64 square = lsb(v);
					int piece = Square(square);
					int katt = 0;

					if (piece == IPawn(me))
					{
						if (!Square(square + Push(me)))
							IncV(EI.score, SelfPawnPin);
					}
					else if ((piece & 1) == me)
					{
						IncV(EI.score, SelfPiecePin);
						katt = 1;
					}
					else if (piece != IPawn(opp))
					{
						if (piece < IRook(opp))
						{
							IncV(EI.score, WeakPin);

							if (!(Current->patt[opp] & v))
								katt = 1;
						}

						else if (piece == IQueen(opp))
							IncV(EI.score, ThreatPin);
					}

					if (katt && !(att & PVarC(EI, area, opp)))
						PVarC(EI, king_att, me) += KingAttack;
				}

				else if (v == (v &(Minor(opp) | Queen(opp))))
					IncV(EI.score, RKingRay);

		if (att & PVarC(EI, area, opp))
		{
			PVarC(EI, king_att, me) += KingRAttack;

			for (uint64 v = att & PVarC(EI, area, opp); T(v); Cut(v))
				if (FullLine[sq][lsb(v)] & att & Major(me))
					PVarC(EI, king_att, me)++;
		}

		IncV(EI.score, Mobility[PieceType[WhiteRook] - 1][popcount<HPopCnt>(att & PVarC(EI, free, me))]);

		if (att & PVarC(EI, free, me) & Pawn(opp))
			IncV(EI.score, TacticalMajorPawn);

		if (att & PVarC(EI, free, me) & Minor(opp))
			IncV(EI.score, TacticalMajorMinor);

		Current->threat |= att & Queen(opp);

		if (!(PWay[me][sq] & Pawn(me)))
		{
			IncV(EI.score, RookHof);
			int hof_score = 0;

			if (!(PWay[me][sq] & Pawn(opp)))
			{
				IncV(EI.score, RookOf);

				if (att & Line(me, 7))
					hof_score += RookOfOpen;
				else if (uint64 target = att & PWay[me][sq] & Minor(opp))
				{
					if (!(Current->patt[opp] & target))
					{
						hof_score += RookOfMinorHanging;

						if (PWay[me][sq] & King(opp))
							hof_score += RookOfKingAtt;
					}
					else
						hof_score += RookOfMinorFixed;
				}
			}
			else if (att & PWay[me][sq] & Pawn(opp))
			{
				uint64 square = lsb(att & PWay[me][sq] & Pawn(opp));

				if (!(PSupport[opp][square] & Pawn(opp)))
					hof_score += RookHofWeakPAtt;
			}
			IncV(EI.score, hof_score);

			if (PWay[opp][sq] & att & Major(me))
				IncV(EI.score, hof_score);
		}

		if ((b & Line(me, 6)) && ((King(opp) | Pawn(opp)) & (Line(me, 6) | Line(me, 7))))
		{
			IncV(EI.score, Rook7th);

			if (King(opp) & Line(me, 7))
				IncV(EI.score, Rook7thK8th);

			if (Major(me) & att & Line(me, 6))
				IncV(EI.score, Rook7thDoubled);
		}
	}
}

template <int me, bool HPopCnt> static __forceinline void eval_bishops(GEvalInfo &EI)
{
	uint64 u = 0, b = 0;

	for (u = Bishop(me); T(u); u ^= b)
	{
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = BishopAttacks(sq, EI.occ);
		Current->att[me] |= att;

		if (BMask[sq] & King(opp))
			if (uint64 v = Between[PVarC(EI, king, opp)][sq] & EI.occ)
				if (Single(v))
				{
					Current->xray[me] |= v;
					uint64 square = lsb(v);
					int piece = Square(square);
					int katt = 0;

					if (piece == IPawn(me))
					{
						if (!Square(square + Push(me)))
							IncV(EI.score, SelfPawnPin);
					}
					else if ((piece & 1) == me)
					{
						IncV(EI.score, SelfPiecePin);
						katt = 1;
					}
					else if (piece != IPawn(opp))
					{
						if (piece < ILight(opp))
						{
							IncV(EI.score, StrongPin);

							if (!(Current->patt[opp] & v))
								katt = 1;
						}

						else if (piece >= IRook(opp))
							IncV(EI.score, ThreatPin);
					}

					if (katt && !(att & PVarC(EI, area, opp)))
						PVarC(EI, king_att, me) += KingAttack;
				}

				else if (v == (v &(Knight(opp) | Major(opp))))
					IncV(EI.score, BKingRay);

		if (att & PVarC(EI, area, opp))
			PVarC(EI, king_att, me) += KingBAttack;
		IncV(EI.score, Mobility[PieceType[WhiteLight] - 1][popcount<HPopCnt>(att & PVarC(EI, free, me))]);

		if (att & PVarC(EI, free, me) & Pawn(opp))
			IncV(EI.score, TacticalMinorPawn);

		if (att & PVarC(EI, free, me) & Knight(opp))
			IncV(EI.score, TacticalMinorMinor);

		if (att & PVarC(EI, area, me))
			IncV(EI.score, KingDefBishop);
		Current->threat |= att & Major(opp);

		if (b & LightArea)
		{
			uint64 v = BishopForward[me][sq] & Pawn(me) & LightArea;
			v |= (v &(File[2] | File[3] | File[4] | File[5] | BMask[sq])) >> 8;
			DecV(EI.score, BishopPawnBlock * popcount<HPopCnt>(v));
		}
		else
		{
			uint64 v = BishopForward[me][sq] & Pawn(me) & DarkArea;
			v |= (v &(File[2] | File[3] | File[4] | File[5] | BMask[sq])) >> 8;
			DecV(EI.score, BishopPawnBlock * popcount<HPopCnt>(v));
		}
	}
}

template <int me, bool HPopCnt> static __forceinline void eval_knights(GEvalInfo &EI)
{
	uint64 u = 0, b = 0;

	for (u = Knight(me); T(u); u ^= b)
	{
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = NAtt[sq];
		Current->att[me] |= att;

		if (att & PVarC(EI, area, opp))
			PVarC(EI, king_att, me) += KingNAttack;
		IncV(EI.score, Mobility[PieceType[WhiteKnight] - 1][popcount<HPopCnt>(att & PVarC(EI, free, me))]);

		if (att & PVarC(EI, free, me) & Pawn(opp))
			IncV(EI.score, TacticalMinorPawn);

		if (att & PVarC(EI, free, me) & Bishop(opp))
			IncV(EI.score, TacticalMinorMinor);

		if (att & PVarC(EI, area, me))
			IncV(EI.score, KingDefKnight);
		Current->threat |= att & Major(opp);

		if ((b & Outpost[me]) && !(Pawn(opp) & PIsolated[File(sq)] & Forward[me][Rank(sq)]))
		{
			IncV(EI.score, KnightOutpost);

			if (Current->patt[me] & b)
			{
				IncV(EI.score, KnightOutpostProtected);

				if (att & PVarC(EI, free, me) & Pawn(opp))
					IncV(EI.score, KnightOutpostPawnAtt);

				if (att & PVarC(EI, free, me) & Bishop(opp))
					IncV(EI.score, KnightOutpostBishopAtt);
			}
		}
	}
}

template <int me, bool HPopCnt> static __forceinline void eval_king(GEvalInfo &EI)
{
	int cnt = Opening(PVarC(EI, king_att, me));
	if (cnt > 15)
		cnt = 15;
	int score = Endgame(PVarC(EI, king_att, me));

	if (cnt >= 2 && T(Queen(me)))
	{
		score += (EI.PawnEntry->shelter[opp] * KingShelterQuad) >> 6;

		if (uint64 u = Current->att[me] & PVarC(EI, area, opp) & (~Current->att[opp]))
			score += popcount<HPopCnt>(u) * KingAttackSquare;

		if (!(SArea[PVarC(EI, king, opp)] & (~(Piece(opp) | Current->att[me]))))
			score += KingNoMoves;
	}
	int adjusted = ((score * KingAttackScale[cnt]) >> 3) + EI.PawnEntry->shelter[opp];

	if (!Queen(me))
		adjusted /= 2;
	IncV(EI.score, adjusted);
}

template <int me, bool HPopCnt> static __forceinline void eval_passer(GEvalInfo &EI)
{
	for (uint64 u = EI.PawnEntry->passer[me]; T(u); Cut(u))
	{
		int file = lsb(u);
		int sq = NB(opp, File[file] & Pawn(me));
		int rank = CRank(me, sq);
		Current->passer |= Bit(sq);

		if (rank <= 2)
			continue;

		if (!Square(sq + Push(me)))
			IncV(EI.score, PasserBlocked[rank]);
		uint64 way = PWay[me][sq];
		int connected = 0, supported = 0, hooked = 0, unsupported = 0, free = 0;

		if (!(way & Piece(opp)))
		{
			IncV(EI.score, PasserClear[rank]);

			if (PWay[opp][sq] & Major(me))
			{
				int square = NB(opp, PWay[opp][sq] & Major(me));

				if (F(Between[sq][square] & EI.occ))
					supported = 1;
			}

			if (PWay[opp][sq] & Major(opp))
			{
				int square = NB(opp, PWay[opp][sq] & Major(opp));

				if (F(Between[sq][square] & EI.occ))
					hooked = 1;
			}

			for (uint64 v = PAtt[me][sq - Push(me)] & Pawn(me); T(v); Cut(v))
			{
				int square = lsb(v);

				if (F(Pawn(opp) & (File[File(square)] | PIsolated[File(square)]) & Forward[me][Rank(square)]))
					connected++;
			}

			if (connected)
				IncV(EI.score, PasserConnected[rank]);

			if (!hooked && !(Current->att[opp] & way))
			{
				IncV(EI.score, PasserFree[rank]);
				free = 1;
			}
			else
			{
				uint64 attacked = Current->att[opp] | (hooked ? way : 0);

				if (supported || (!hooked && connected) || (!(Major(me) & way) && !(attacked &(~Current->att[me]))))
					IncV(EI.score, PasserSupported[rank]);
				else
					unsupported = 1;
			}
		}

		if (rank == 6)
		{
			if ((way & Rook(me)) && !Minor(me) && !Queen(me) && Single(Rook(me)))
				DecV(EI.score, PasserOpRookBlock);
		}
	}
}

template <int me, bool HPopCnt> static __forceinline void eval_pieces(GEvalInfo &EI)
{
	Current->threat |= Current->att[opp] & (~Current->att[me]) & Piece(me);

	if (uint64 u = Current->threat & Piece(me))
	{
		DecV(EI.score, TacticalThreat);
		Cut(u);

		if (u)
		{
			DecV(EI.score, TacticalThreat + TacticalDoubleThreat);

			for (Cut(u); u; Cut(u))
				DecV(EI.score, TacticalThreat);
		}
	}
}

template <bool HPopCnt> static void eval_unusual_material(GEvalInfo &EI)
{
	int wp = popcount<HPopCnt>(Pawn(White));
	int bp = popcount<HPopCnt>(Pawn(Black));
	int wlight = popcount<HPopCnt>(Minor(White));
	int blight = popcount<HPopCnt>(Minor(Black));
	int wr = popcount<HPopCnt>(Rook(White));
	int br = popcount<HPopCnt>(Rook(Black));
	int wq = popcount<HPopCnt>(Queen(White));
	int bq = popcount<HPopCnt>(Queen(Black));
	int phase = Min(24, (wlight + blight) + 2 * (wr + br) + 4 * (wq + bq));
	int mat_score =
		SeeValue[WhitePawn] * (wp - bp) + SeeValue[WhiteKnight] * (wlight - blight) + SeeValue[WhiteRook] * (wr - br)
		+ SeeValue[WhiteQueen] * (wq - bq);
	mat_score = Compose(mat_score, mat_score);
	Current->score = (((Opening(mat_score + EI.score) * phase) + (Endgame(mat_score + EI.score) * (24 - phase))) / 24);

	if (Current->turn)
		Current->score = -Current->score;
	UpdateDelta
}

#endif