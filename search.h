#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

static GBoard SaveBoard[1];
static GData SaveData[1];
int PrevMove;
static int Previous;
static int save_sp;
static uint64 check_node;
static uint64 check_node_smp;

enum
{
	stage_search,
	s_hash_move,
	s_good_cap,
	s_special,
	s_quiet,
	s_bad_cap,
	s_none,
	stage_evasion,
	e_hash_move,
	e_ev,
	e_none,
	stage_razoring,
	r_hash_move,
	r_cap,
	r_checks,
	r_none
};

static __forceinline int MaxF(int x, int y)
{
	return Max(x, y);
}


static __forceinline double MaxF(double x, double y)
{
	return Max(x, y);
}

template <int me, bool exclusion> int search(int beta, int depth, int flags)
{
	int i = 0, value = 0, cnt = 0, flag = 0, moves_to_play = 0, check, score = 0, move = 0, ext = 0, margin = 0;
	int hash_move = 0, do_split = 0, sp_init = 0, singular = 0, played = 0, high_depth = 0, high_value = 0;
	int hash_value = 0, new_depth = 0, move_back = 0, hash_depth = 0;
	int height = (int)(Current - Data);
	int * p = NULL;
	GSP* Sp = NULL;

	if (nodes > check_node_smp + 0x10)
	{

#ifndef W32_BUILD
		InterlockedAdd64(&Smpi->nodes, (long long)(nodes)-(long long)(check_node_smp));
#else
		Smpi->nodes += (long long)(nodes)-(long long)(check_node_smp);
#endif

		check_node_smp = nodes;
		check_state();

		if (nodes > check_node + 0x4000 && parent)
		{
			check_node = nodes;
			check_time(1);

			if (Searching)
				SET_BIT_64(Smpi->searching, Id);
		}
	}

	if (depth <= 1)
		return q_search<me, 0>(beta - 1, beta, 1, flags);

	if (flags & FlagHaltCheck)
	{
		if (height - MateValue >= beta)
			return beta;

		if (MateValue - height < beta)
			return beta - 1;
		halt_check;
	}

	if (TBs_initialized)
	{
		if (height == 1 && parent && depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			int success;
			value = probe_distance(&success);

			if (success)
				return value;
		}
	}

	if (exclusion)
	{
		cnt = high_depth = do_split = sp_init = singular = played = 0;
		flag = 1;
		score = beta - 1;
		high_value = MateValue;
		hash_value = -MateValue;
		hash_depth = -1;
		hash_move = flags & 0xFFFF;
		goto skip_hash_move;
	}

	if (flags & FlagCallEvaluation)
		evaluate();

	if (Check(me))
		return search_evasion<me, 0>(beta, depth, flags &(~(FlagHaltCheck | FlagCallEvaluation)));

	if ((value = Current->score - s_vs - (depth << 3) - (Max(depth - s_vmds, 0) << 5)) >= beta
		&& F(Pawn(opp) & Line(me, 1) & Shift(me, ~PieceAll)) && T(NonPawnKing(me))
		&& F(flags &(FlagReturnBestMove | FlagDisableNull)) && depth <= s_vmaxd)
		return value;

	if ((value = Current->score + s_va) < beta && depth <= s_qsdmax)
		return MaxF(value, q_search<me, 0>(beta - 1, beta, 1, FlagHashCheck | (flags & 0xFFFF)));

	high_depth = 0;
	high_value = MateValue;
	hash_value = -MateValue;
	hash_depth = -1;
	Current->best = hash_move = flags & 0xFFFF;

	if (GEntry*Entry = probe_hash())
	{
		if (Entry->high_depth > high_depth)
		{
			high_depth = Entry->high_depth;
			high_value = Entry->high;
		}

		if (Entry->high < beta && Entry->high_depth >= depth)
			return Entry->high;

		if (T(Entry->move) && Entry->low_depth > hash_depth)
		{
			Current->best = hash_move = Entry->move;
			hash_depth = Entry->low_depth;

			if (Entry->low_depth)
				hash_value = Entry->low;
		}

		if (Entry->low >= beta && Entry->low_depth >= depth)
		{
			if (Entry->move)
			{
				Current->best = Entry->move;

				if (F(Square(To(Entry->move))) && F(Entry->move & 0xE000))
				{
					if (Current->killer[1] != Entry->move && F(flags & FlagNoKillerUpdate))
					{
						Current->killer[2] = Current->killer[1];
						Current->killer[1] = Entry->move;
					}
					UpdateRef(Entry->move);
				}
				return Entry->low;
			}

			if (F(flags & FlagReturnBestMove))
				return Entry->low;
		}
	}

	if (depth >= s_dmin)
		if (GPVEntry*PVEntry = probe_pv_hash())
		{
			hash_low(PVEntry->move, PVEntry->value, PVEntry->depth);
			hash_high(PVEntry->value, PVEntry->depth);

			if (PVEntry->depth >= depth)
			{
				if (PVEntry->move)
					Current->best = PVEntry->move;

				if (F(flags & FlagReturnBestMove)
					&& ((Current->ply <= 50 && PVEntry->ply <= 50) || (Current->ply >= 50 && PVEntry->ply >= 50)))
					return PVEntry->value;
			}

			if (T(PVEntry->move) && PVEntry->depth > hash_depth)
			{
				Current->best = hash_move = PVEntry->move;
				hash_depth = PVEntry->depth;
				hash_value = PVEntry->value;
			}
		}

	if (depth < s_maxd)
		score = height - MateValue;
	else
		score = beta - 1;

	if (TBs_initialized)
	{
		if (depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			int success;
			value = probe_win(&success);

			if (success)
			{
				if (value == 0)
					return 0;

				if (value > EvalValue)
				{
					if (value >= beta)
						return value;
					score = Max(score, value);
				}

				else
				{
					if (value < -EvalValue && value < beta)
						return value;
				}
			}
		}
	}

	if (depth >= s_rsmind && (F(hash_move) || hash_value < beta || hash_depth < depth - s_rsmind)
		&& (high_value >= beta || high_depth < depth - s_rsmind) && F(flags & FlagDisableNull))
	{
		new_depth = depth - s_nds;
		value = search<me, 0>(beta, new_depth,
			FlagHashCheck | FlagNoKillerUpdate | FlagDisableNull | FlagReturnBestMove | hash_move);

		if (value >= beta)
		{
			if (Current->best)
				hash_move = Current->best;
			hash_depth = new_depth;
			hash_value = beta;
		}
	}

	if (depth >= s_minnd && Current->score + s_csa >= beta && F(flags &(FlagDisableNull | FlagReturnBestMove))
		&& (high_value >= beta || high_depth < depth - s_hdds)
		&& (depth < s_nmdmax || (hash_value >= beta && hash_depth >= depth - s_hds)) && beta > -EvalValue && T(NonPawnKing(me)))
	{
		new_depth = depth - s_nds;
		do_null();
		value = -search<opp, 0>(1 - beta, new_depth, FlagHashCheck);
		undo_null();

		if (value >= beta)
		{
			if (depth < s_vgbdmax)
				hash_low(0, value, depth);
			return value;
		}
	}

	cnt = flag = singular = played = 0;

	if (T(hash_move) && is_legal<me>(move = hash_move))
	{
		if (IsIllegal(me, move))
			goto skip_hash_move;
		cnt++;
		check = is_check<me>(move);

		if (check)
			ext = 1 + (depth < s_cedmax);
		else
			ext = extension<0>(move, depth);

		if (depth >= s_hmemd && hash_value >= beta && hash_depth >= (new_depth = depth - Min(s_hmeds, depth >> 1)))
		{
			int margin_one = beta - ExclSingle(depth);
			int margin_two = beta - ExclDouble(depth);
			int prev_ext = Ext(flags);
			singular = singular_extension<me>(ext, prev_ext, margin_one, margin_two, new_depth, hash_move);

			if (singular)
				ext = Max(ext, singular + (prev_ext < 1) - (singular >= s_sesmax && prev_ext >= s_sepemin));
		}

		if (depth < s_dmeme && To(move) == To(Current->move) && T(Square(To(move))))
			ext = Max(ext, s_meb);
		new_depth = depth - s_hmnds + ext;
		do_move<me>(move);
		value = -search<opp, 0>(1 - beta, new_depth, FlagNeatSearch
			| ((hash_value >= beta && hash_depth >= depth - s_rsmds) ? FlagDisableNull : 0) | ExtFlag(ext));
		undo_move<me>(move);
		played++;

		if (value > score)
		{
			score = value;

			if (value >= beta)
				goto cut;
		}
	}

skip_hash_move:
	Current->killer[0] = 0;
	Current->stage = stage_search;
	Current->gen_flags = 0;
	Current->ref[0] = RefM(Current->move).ref[0];
	Current->ref[1] = RefM(Current->move).ref[1];
	move_back = 0;

	if (beta > 0 && Current->ply >= 2)
	{
		if (F((Current - 1)->move & 0xF000))
		{
			move_back = (To((Current - 1)->move) << 6) | From((Current - 1)->move);

			if (Square(To(move_back)))
				move_back = 0;
		}
	}
	moves_to_play = 3 + (depth * depth) / 6;
	margin = Current->score + s_csm + (depth << 3) + (Max(depth - s_mmds, 0) << 5);

	if ((value = margin) < beta && depth <= s_vemdmax)
	{
		flag = 1;
		score = Max(value, score);
		Current->stage = stage_razoring;
		Current->mask = Piece(opp);

		if ((value = Current->score + s_vcsrm + (depth << 5)) < beta)
		{
			score = Max(value, score);
			Current->mask ^= Pawn(opp);
		}
	}
	Current->current = Current->moves;
	Current->moves[0] = 0;

	if (depth <= s_cgfmd)
		Current->gen_flags |= FlagNoBcSort;

	do_split = sp_init = 0;

	if (depth >= SplitDepth && PrN > 1 && parent && !exclusion)
		do_split = 1;

	while (move = get_move<me, 0>())
	{
		if (move == hash_move)
			continue;

		if (IsIllegal(me, move))
			continue;
		cnt++;

		if (move == move_back)
		{
			score = Max(0, score);
			continue;
		}

		if (Current->stage == r_checks)
			check = 1;
		else
			check = is_check<me>(move);

		if (T(check) && T(see<me>(move, 0)))
			ext = 1 + (depth < s_csedmax);
		else
			ext = extension<0>(move, depth);
		new_depth = depth - s_ndse + ext;

		if (F(Square(To(move))) && F(move & 0xE000))
		{
			if (move != Current->killer[1] && move != Current->killer[2])
			{
				if (F(check) && cnt > moves_to_play)
				{
					Current->gen_flags &= ~FlagSort;
					continue;
				}

				if (depth >= s_rmd)
				{
					int reduction = msb(cnt);

					if (move == Current->ref[0] || move == Current->ref[1])
						reduction = Max(0, reduction - 1);

					if (reduction >= 2 && !(Queen(White) | Queen(Black)) && popcnt(NonPawnKingAll) <= 4)
						reduction += reduction >> 1;

					if (new_depth - reduction > s_ndrmin)
						if (F(see<me>(move, -s_smv)))
							reduction += s_smra;

					if (T(reduction) && reduction < s_rmax && new_depth - reduction > s_ndrmin)
					{
						if (cnt > s_rmc)
							reduction = s_cgtr;
						else
							reduction = 0;
					}
					new_depth = Max(s_ndmax, new_depth - reduction);
				}
			}

			if (F(check))
			{
				if ((value = Current->score + DeltaM(move) + s_ncva) < beta && depth <= s_ncmd)
				{
					score = Max(value, score);
					continue;
				}

				if (cnt > s_nccmin && (value = margin + DeltaM(move) - s_msbcmult * msb(cnt)) < beta && depth <= s_ncdmax)
				{
					score = Max(value, score);
					continue;
				}
			}

			if (depth <= s_cdmin && T(NonPawnKing(me)) && F(see<me>(move, -s_smv2)))
				continue;
		}
		else
		{
			if (Current->stage == r_cap)
			{
				if (F(check) && depth <= s_csrcdmin && F(see<me>(move, -s_smv3)))
					continue;
			}

			else if (Current->stage == s_bad_cap && F(check) && depth <= s_csbcdmax)
				continue;
		}

		if (do_split && played >= 1)
		{
			if (!sp_init)
			{
				sp_init = 1;
				uint64 u = ~Smpi->active_sp;

				if (!u)
				{
					do_split = 0;
					goto make_move;
				}
				Sp = &Smpi->sp[lsb(u)];
				init_sp(Sp, beta - 1, beta, depth, 0, singular, height);
			}
			GMove* M = &Sp->move[Sp->move_number++];
			M->ext = ext;
			M->flags = 0;
			M->move = move;
			M->reduced_depth = new_depth;
			M->research_depth = depth - s_Mrds + ext;
			M->stage = Current->stage;
			continue;
		}
	make_move:
		do_move<me>(move);
		value = -search<opp, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));

		if (value >= beta && new_depth < depth - 2 + ext)
			value = -search<opp, 0>(1 - beta, depth - 2 + ext, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		undo_move<me>(move);
		played++;

		if (value > score)
		{
			score = value;

			if (value >= beta)
				goto cut;
		}
	}

	if (do_split && sp_init)
	{
		value = smp_search<me>(Sp);

		if (value >= beta && Sp->best_move)
		{
			score = beta;
			Current->best = move = Sp->best_move;

			for (i = 0; i < Sp->move_number; i++)
			{
				GMove* M = &Sp->move[i];

				if ((M->flags & FlagFinished) && M->stage == s_quiet && M->move != move)
					HistoryBad(M->move);
			}
		}

		if (value >= beta)
			goto cut;
	}

	if (F(cnt) && F(flag))
	{
		hash_high(0, 127);
		hash_low(0, 0, 127);
		return 0;
	}

	if (F(exclusion))
		hash_high(score, depth);
	return score;
cut:
	if (exclusion)
		return score;

	Current->best = move;

	if (depth >= s_mbsdmax)
		score = Min(beta, score);
	hash_low(move, score, depth);

	if (F(Square(To(move))) && F(move & 0xE000))
	{
		if (Current->killer[1] != move && F(flags & FlagNoKillerUpdate))
		{
			Current->killer[2] = Current->killer[1];
			Current->killer[1] = move;
		}
		HistoryGood(move);

		if (move != hash_move && Current->stage == s_quiet && !sp_init)
			for (p = Current->start; p < (Current->current - 1); p++)
				HistoryBad(*p);
		UpdateRef(move);
	}
	return score;
}

template <int me, bool pv> static int q_search(int alpha, int beta, int depth, int flags)
{
	int i = 0, value = 0, score = 0, move = 0, hash_move = 0, hash_depth = 0, cnt = 0, success = 0;
	GEntry* Entry = NULL;

	if (flags & FlagHaltCheck)
		halt_check;

	if (TBs_initialized)
	{
		if (depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			if ((int)(Current - Data) == 1 && parent)
			{
				value = probe_distance(&success);

				if (success)
					return value;
			}
			value = probe_win(&success);
			if (success)
			{
				if (value == 0)
					return 0;

				if (value > EvalValue)
				{
					if (value >= beta)
						return value;
					alpha = Max(alpha, value);
				}
				else
				{
					if (value < -EvalValue && value <= alpha)
						return value;
				}
			}
		}
	}

	if (flags & FlagCallEvaluation)
		evaluate();

	if (Check(me))
		return q_evasion<me, pv>(alpha, beta, depth, FlagHashCheck);
	score = Current->score + qs_secsm;

	if (score > alpha)
	{
		alpha = score;

		if (score >= beta)
			return score;
	}

	hash_move = hash_depth = 0;

	if (flags & FlagHashCheck)
	{
		for (i = 0, Entry = Hash + (High32(Current->key) & hash_mask); i < 4; Entry++, i++)
		{
			if (Low32(Current->key) == Entry->key)
			{
				if (T(Entry->low_depth))
				{
					if (Entry->low >= beta && !pv)
						return Entry->low;

					if (Entry->low_depth > hash_depth && T(Entry->move))
					{
						hash_move = Entry->move;
						hash_depth = Entry->low_depth;
					}
				}

				if (T(Entry->high_depth) && Entry->high <= alpha && !pv)
					return Entry->high;
				break;
			}
		}
	}

	Current->mask = Piece(opp);
	capture_margin<me>(alpha, score);

	cnt = 0;

	if (T(hash_move))
	{
		if (F(Bit(To(hash_move)) & Current->mask) && F(hash_move & 0xE000)
			&& (depth < -qs_hmmd || (Current->score + DeltaM(hash_move) <= alpha && F(is_check<me>(hash_move)))))
			goto skip_hash_move;

		if (is_legal<me>(move = hash_move))
		{
			if (IsIllegal(me, move))
				goto skip_hash_move;

			if (SeeValue[Square(To(move))] > SeeValue[Square(From(move))])
				cnt++;
			do_move<me>(move);
			value = -q_search<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
			undo_move<me>(move);

			if (value > score)
			{
				score = value;

				if (value > alpha)
				{
					alpha = value;

					if (value >= beta)
						goto cut;
				}
			}

			if (F(Bit(To(hash_move)) & Current->mask) && F(hash_move & 0xE000)
				&& (depth < -2 || depth <= -1 && Current->score + qs_csm < alpha) && alpha >= beta - 1 && !pv)
				return alpha;
		}
	}

skip_hash_move:
	gen_captures <me, 0>(Current->moves);
	Current->current = Current->moves;

	while (move = pick_move())
	{
		if (move == hash_move)
			continue;

		if (IsIllegal(me, move))
			continue;

		if (F(see<me>(move, -qs_smv)))
			continue;

		if (SeeValue[Square(To(move))] > SeeValue[Square(From(move))])
			cnt++;
		do_move<me>(move);
		value = -q_search<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		undo_move<me>(move);

		if (value > score)
		{
			score = value;

			if (value > alpha)
			{
				alpha = value;

				if (value >= beta)
					goto cut;
			}
		}
	}

	if (depth < -2)
		goto finish;

	if (depth <= -1 && Current->score + qs_csm < alpha)
		goto finish;

	gen_checks<me>(Current->moves);
	Current->current = Current->moves;

	while (move = pick_move())
	{
		if (move == hash_move)
			continue;

		if (IsIllegal(me, move))
			continue;

		if (IsRepetition(alpha + 1, move))
			continue;

		if (F(see<me>(move, -qs_smv)))
			continue;
		do_move<me>(move);
		value = -q_evasion<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		undo_move<me>(move);

		if (value > score)
		{
			score = value;

			if (value > alpha)
			{
				alpha = value;

				if (value >= beta)
					goto cut;
			}
		}
	}

	if (T(cnt) || Current->score
		+ qs_csa < alpha || T(Current->threat & Piece(me)) || T((Current->xray[opp] | Current->pin[opp]) & NonPawn(opp))
		|| T(Pawn(opp) & Line(me, 1) & Shift(me, ~PieceAll)))
		goto finish;

	Current->margin = alpha - Current->score + qs_cmcss;
	gen_delta_moves<me>(Current->moves);
	Current->current = Current->moves;

	while (move = pick_move())
	{
		if (move == hash_move)
			continue;

		if (IsIllegal(me, move))
			continue;

		if (IsRepetition(alpha + 1, move))
			continue;

		if (F(see<me>(move, -qs_smv)))
			continue;
		cnt++;
		do_move<me>(move);
		value = -q_search<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		undo_move<me>(move);

		if (value > score)
		{
			score = value;

			if (value > alpha)
			{
				alpha = value;

				if (value >= beta)
				{
					if (Current->killer[1] != move)
					{
						Current->killer[2] = Current->killer[1];
						Current->killer[1] = move;
					}
					goto cut;
				}
			}
		}

		if (cnt >= 3)
			break;
	}

finish:
	if (depth >= -2 && (depth >= 0 || Current->score + qs_csgtam >= alpha))
		hash_high(score, 1);
	return score;
cut:
	hash_low(move, score, 1);
	return score;
}

template <int me, bool pv> static int q_evasion(int alpha, int beta, int depth, int flags)
{
	int i = 0, value = 0, pext = 0, move = 0, cnt = 0, hash_move = 0, hash_depth = 0, success = 0;
	int score = Convert((Current - Data), int) - MateValue;
	int* p = NULL;
	GEntry* Entry = NULL;

	if (flags & FlagHaltCheck)
		halt_check;

	if (TBs_initialized)
	{
		if (depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			if ((int)(Current - Data) == 1 && parent)
			{
				value = probe_distance(&success);
				if (success)
					return value;
			}

			value = probe_win(&success);
			if (success)
			{
				if (value == 0)
					return 0;

				if (value > EvalValue)
				{
					if (value >= beta)
						return value;
					alpha = Max(alpha, value);
				}

				else
				{
					if (value < -EvalValue && value <= alpha)
						return value;
				}
			}
		}
	}

	hash_move = hash_depth = 0;

	if (flags & FlagHashCheck)
	{
		for (i = 0, Entry = Hash + (High32(Current->key) & hash_mask); i < 4; Entry++, i++)
		{
			if (Low32(Current->key) == Entry->key)
			{
				if (T(Entry->low_depth))
				{
					if (Entry->low >= beta && !pv)
						return Entry->low;

					if (Entry->low_depth > hash_depth && T(Entry->move))
					{
						hash_move = Entry->move;
						hash_depth = Entry->low_depth;
					}
				}

				if (T(Entry->high_depth) && Entry->high <= alpha && !pv)
					return Entry->high;
				break;
			}
		}
	}

	if (flags & FlagCallEvaluation)
		evaluate();
	Current->mask = Filled;

	if (Current->score - qe_css <= alpha && !pv)
	{
		Current->mask = Piece(opp);
		score = Current->score - qe_css;
		capture_margin<me>(alpha, score);
	}

	alpha = Max(score, alpha);
	pext = 0;
	gen_evasions<me>(Current->moves);
	Current->current = Current->moves;

	if (F(Current->moves[0]))
		return score;

	if (F(Current->moves[1]))
		pext = 1;
	else
	{
		Current->ref[0] = RefM(Current->move).check_ref[0];
		Current->ref[1] = RefM(Current->move).check_ref[1];
		mark_evasions(Current->moves);

		if (T(hash_move) && (T(Bit(To(hash_move)) & Current->mask) || T(hash_move & 0xE000)))
		{
			for (p = Current->moves; T(*p); p++)
			{
				if (((*p) & 0xFFFF) == hash_move)
				{
					*p |= 0x7FFF0000;
					break;
				}
			}
		}
	}
	cnt = 0;

	while (move = pick_move())
	{
		if (IsIllegal(me, move))
			continue;
		cnt++;

		if (IsRepetition(alpha + 1, move))
		{
			score = Max(0, score);
			continue;
		}

		if (F(Square(To(move))) && F(move & 0xE000))
		{
			if (cnt > 3 && F(is_check<me>(move)) && !pv)
				continue;

			if ((value = Current->score + DeltaM(move) + qe_dma) <= alpha && !pv)
			{
				score = Max(value, score);
				continue;
			}
		}
		do_move<me>(move);
		value = -q_search<opp, pv>(-beta, -alpha, depth - 1 + pext, FlagNeatSearch);
		undo_move<me>(move);

		if (value > score)
		{
			score = value;

			if (value > alpha)
			{
				alpha = value;

				if (value >= beta)
					goto cut;
			}
		}
	}

	return score;
cut:
	return score;
}

template <int me, bool exclusion> static int search_evasion(int beta, int depth, int flags)
{
	int i = 0, value = 0, score = 0, pext = 0, move = 0, cnt = 0;
	int hash_depth = 0, hash_move = 0, new_depth = 0, ext = 0, check = 0, moves_to_play = 0;
	int hash_value = -MateValue;
	int height = (int)(Current - Data);

	if (depth <= 1)
		return q_evasion<me, 0>(beta - 1, beta, 1, flags);
	score = height - MateValue;

	if (flags & FlagHaltCheck)
	{
		if (score >= beta)
			return beta;

		if (MateValue - height < beta)
			return beta - 1;
		halt_check;
	}

	if (TBs_initialized)
	{
		if (height == 1 && parent && depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			int success;
			value = probe_distance(&success);

			if (success)
				return value;
		}
	}

	hash_depth = -1;
	hash_move = flags & 0xFFFF;

	if (exclusion)
	{
		cnt = pext = 0;
		score = beta - 1;
		gen_evasions<me>(Current->moves);

		if (F(Current->moves[0]))
			return score;
		goto skip_hash_move;
	}
	Current->best = hash_move;

	if (GEntry*Entry = probe_hash())
	{
		if (Entry->high < beta && Entry->high_depth >= depth)
			return Entry->high;

		if (T(Entry->move) && Entry->low_depth > hash_depth)
		{
			Current->best = hash_move = Entry->move;
			hash_depth = Entry->low_depth;
		}

		if (Entry->low >= beta && Entry->low_depth >= depth)
		{
			if (Entry->move)
			{
				Current->best = Entry->move;

				if (F(Square(To(Entry->move))) && F(Entry->move & 0xE000))
					UpdateCheckRef(Entry->move);
			}
			return Entry->low;
		}

		if (Entry->low_depth >= depth - se_ldmds && Entry->low > hash_value)
			hash_value = Entry->low;
	}

	if (depth >= se_md)
	{
		if (GPVEntry*PVEntry = probe_pv_hash())
		{
			hash_low(PVEntry->move, PVEntry->value, PVEntry->depth);
			hash_high(PVEntry->value, PVEntry->depth);

			if (PVEntry->depth >= depth)
			{
				if (PVEntry->move)
					Current->best = PVEntry->move;
				return PVEntry->value;
			}

			if (T(PVEntry->move) && PVEntry->depth > hash_depth)
			{
				Current->best = hash_move = PVEntry->move;
				hash_depth = PVEntry->depth;
				hash_value = PVEntry->value;
			}
		}
	}
	if (hash_depth >= depth && hash_value > -EvalValue)
		score = Min(beta - 1, Max(score, hash_value));

	if (TBs_initialized)
	{
		if (depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			int success;
			value = probe_win(&success);

			if (success)
			{
				if (value == 0)
					return 0;

				if (value > EvalValue)
				{
					if (value >= beta)
						return value;
					score = Max(score, value);
				}

				else
				{
					if (value < -EvalValue && value < beta)
						return value;
				}
			}
		}
	}

	if (flags & FlagCallEvaluation)
		evaluate();

	Current->mask = Filled;

	if (Current->score - se_css < beta && depth <= se_csdmax)
	{
		Current->mask = Piece(opp);
		score = Current->score - se_css;
		capture_margin<me>(beta, score);
	}
	cnt = 0;
	pext = 0;
	gen_evasions<me>(Current->moves);

	if (F(Current->moves[0]))
		return score;

	if (F(Current->moves[1]))
		pext = 2;

	if (T(hash_move) && is_legal<me>(move = hash_move))
	{
		if (IsIllegal(me, move))
			goto skip_hash_move;
		cnt++;
		check = is_check<me>(move);

		if (check)
			ext = Max(pext, 1 + (depth < se_cedmax));
		else
			ext = MaxF(pext, extension<0>(move, depth));

		if (depth >= se_dmin && hash_value >= beta && hash_depth >= (new_depth = depth - Min(se_ndmds, depth >> 1)))
		{
			int margin_one = beta - ExclSingle(depth);
			int margin_two = beta - ExclDouble(depth);
			int prev_ext = Ext(flags);
			int singular = singular_extension<me>(ext, prev_ext, margin_one, margin_two, new_depth, hash_move);

			if (singular)
				ext = Max(ext, singular + (prev_ext < 1) - (singular >= 2 && prev_ext >= 2));
		}
		new_depth = depth - 2 + ext;
		do_move<me>(move);
		evaluate();

		if (Current->att[opp] & King(me))
		{
			undo_move<me>(move);
			cnt--;
			goto skip_hash_move;
		}
		value = -search<opp, 0>(1 - beta, new_depth,
			FlagHaltCheck | FlagHashCheck | ((hash_value >= beta && hash_depth >= depth - 12) ? FlagDisableNull : 0)
			| ExtFlag(ext));
		undo_move<me>(move);

		if (value > score)
		{
			score = value;

			if (value >= beta)
				goto cut;
		}
	}

skip_hash_move:
	moves_to_play = 3 + ((depth * depth) / 6);
	Current->ref[0] = RefM(Current->move).check_ref[0];
	Current->ref[1] = RefM(Current->move).check_ref[1];
	mark_evasions(Current->moves);
	Current->current = Current->moves;

	while (move = pick_move())
	{
		if (move == hash_move)
			continue;

		if (IsIllegal(me, move))
			continue;
		cnt++;

		if (IsRepetition(beta, move))
		{
			score = Max(0, score);
			continue;
		}
		check = is_check<me>(move);

		if (check)
			ext = Max(pext, 1 + (depth < 16));
		else
			ext = MaxF(pext, extension<0>(move, depth));
		new_depth = depth - 2 + ext;

		if (F(Square(To(move))) && F(move & 0xE000))
		{
			if (F(check))
			{
				if (cnt > moves_to_play)
					continue;

				if ((value = Current->score + DeltaM(move) + se_vcsdmm) < beta && depth <= se_vcsdmmd)
				{
					score = Max(value, score);
					continue;
				}
			}

			if (depth >= se_dmd && cnt > se_cmc)
			{
				int reduction = msb(cnt);

				if (reduction >= 2 && !(Queen(White) | Queen(Black)) && popcnt(NonPawnKingAll) <= 4)
					reduction += reduction >> 1;
				new_depth = Max(3, new_depth - reduction);
			}
		}
		do_move<me>(move);
		value = -search<opp, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));

		if (value >= beta && new_depth < depth - 2 + ext)
			value = -search<opp, 0>(1 - beta, depth - 2 + ext, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		undo_move<me>(move);

		if (value > score)
		{
			score = value;

			if (value >= beta)
				goto cut;
		}
	}

	if (F(exclusion))
		hash_high(score, depth);
	return score;
cut:
	if (exclusion)
		return score;

	Current->best = move;
	hash_low(move, score, depth);

	if (F(Square(To(move))) && F(move & 0xE000))
		UpdateCheckRef(move);
	return score;
}

template <int me, bool root> int pv_search(int alpha, int beta, int depth, int flags)
{
	int i = 0, value = 0, move = 0, cnt = 0, pext = 0, ext = 0, check = 0, margin = 0, do_split = 0, sp_init = 0;
	int singular = 0, played = 0, new_depth = 0, hash_move = 0, hash_depth = 0, old_best, ex_depth = 0, ex_value = 0;
	int old_alpha = alpha;
	int start_knodes = int(nodes >> 10);
	int hash_value = -MateValue;
	int height = (int)(Current - Data);
	GSP* Sp;

	if (root)
	{
		depth = Max(depth, 2);
		flags |= ExtFlag(1);

		if (F(RootList[0]))
			return 0;

		if (VerboseUCI)
		{
			fprintf(stdout, "info depth %d\n", (depth / 2));
			fflush(stdout);
		}

		int* p = NULL;

		for (p = RootList; *p; p++);
		sort_moves(RootList, p);

		for (p = RootList; *p; p++)
			*p &= 0xFFFF;
		SetScore(RootList[0], 2);
		goto check_hash;
	}

	if (depth <= 1)
		return q_search<me, 1>(alpha, beta, 1, FlagNeatSearch);

	if (Convert((Current - Data), int) - MateValue >= beta)
		return beta;

	if (MateValue - Convert((Current - Data), int) <= alpha)
		return alpha;
	halt_check;

	if (TBs_initialized)
	{
		if (!root && parent && height == 1 && depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			int success;
			value = probe_distance(&success);

			if (success)
				return value;
		}
	}

check_hash:
	hash_depth = -1;
	Current->best = hash_move = 0;

	if (GPVEntry*PVEntry = probe_pv_hash())
	{
		hash_low(PVEntry->move, PVEntry->value, PVEntry->depth);
		hash_high(PVEntry->value, PVEntry->depth);

		if (PVEntry->depth >= depth && T(PVHashing))
		{
			if (PVEntry->move)
				Current->best = PVEntry->move;

			if ((Current->ply <= 50 && PVEntry->ply <= 50) || (Current->ply >= 50 && PVEntry->ply >= 50))
				if (!PVEntry->value || !draw_in_pv<me>())
					return PVEntry->value;
		}

		if (T(PVEntry->move) && PVEntry->depth > hash_depth)
		{
			Current->best = hash_move = PVEntry->move;
			hash_depth = PVEntry->depth;
			hash_value = PVEntry->value;
		}
	}

	if (GEntry*Entry = probe_hash())
	{
		if (T(Entry->move) && Entry->low_depth > hash_depth)
		{
			Current->best = hash_move = Entry->move;
			hash_depth = Entry->low_depth;

			if (Entry->low_depth)
				hash_value = Entry->low;
		}
	}

	if (TBs_initialized)
	{
		if (!root && depth >= TBProbeDepth && Current->piece_nb <= TBProbeLimit)
		{
			int success;
			value = probe_win(&success);

			if (success)
			{
				if (value == 0)
					return 0;

				if (value > EvalValue)
				{
					if (value >= beta)
						return value;
					alpha = Max(alpha, value);
				}

				else
				{
					if (value < -EvalValue && value <= alpha)
						return value;
				}
			}
		}
	}

	if (root)
	{
		hash_move = RootList[0];
		hash_value = Previous;
		hash_depth = Max(0, depth - 2);
	}

	evaluate();

	if (F(root) && depth >= pvs_dmin && (F(hash_move) || hash_value <= alpha || hash_depth < depth - pvs_hds))
	{
		if (F(hash_move))
			new_depth = depth - pvs_nrnhmds;
		else
			new_depth = depth - pvs_nrhmds;
		value = pv_search<me, 0>(alpha, beta, new_depth, hash_move);

		if (value > alpha)
		{
		hash_move_found:
			if (Current->best)
				hash_move = Current->best;
			hash_depth = new_depth;
			hash_value = value;
			goto skip_iid;
		}
		else
		{
			i = 0;
			new_depth = depth - pvs_nds;
		iid_loop:
			margin = alpha - (pvs_abs << i);

			if (T(hash_move) && hash_depth >= Min(new_depth, depth - pvs_hmds) && hash_value >= margin)
				goto skip_iid;
			value = search<me, 0>(margin, new_depth,
				FlagHashCheck | FlagNoKillerUpdate | FlagDisableNull | FlagReturnBestMove | hash_move);

			if (value >= margin)
				goto hash_move_found;
			i++;

			if (i < pvs_iidlmax)
				goto iid_loop;
		}
	}
skip_iid:
	if (F(root) && Check(me))
	{
		alpha = Max(Convert((Current - Data), int) - MateValue, alpha);
		Current->mask = Filled;
		gen_evasions<me>(Current->moves);

		if (F(Current->moves[0]))
			return Convert((Current - Data), int) - MateValue;

		if (F(Current->moves[1]))
			pext = pvs_pext;
	}

	cnt = 0;

	if (hash_move && is_legal<me>(move = hash_move))
	{
		cnt++;

		if (root)
		{
			memset(Data + 1, 0, 127 * sizeof(GData));
			move_to_string(move, score_string);
			if (VerboseUCI)
				sprintf(info_string, "info currmove %s currmovenumber %d\n", score_string, cnt);
		}
		check = is_check<me>(move);

		if (check)
			ext = pvs_ce;
		else
			ext = MaxF(pext, extension<1>(move, depth));

		if (depth >= pvs_hmdmin && hash_value > alpha && hash_depth >= (new_depth = depth - Min(pvs_hmdmin, depth >> 1)))
		{
			int margin_one = hash_value - ExclSinglePV(depth);
			int margin_two = hash_value - ExclDoublePV(depth);
			int prev_ext = Ext(flags);
			singular = singular_extension
				<me>(root ? 0 : ext, root ? 0 : prev_ext, margin_one, margin_two, new_depth, hash_move);

			if (singular)
			{
				ext = Max(ext, singular + (prev_ext < pvs_spemax) - (singular >= pvs_smin && prev_ext >= pvs_spemin));

				if (root)
					CurrentSI->singular = singular;
				ex_depth = new_depth;
				ex_value = (singular >= pvs_evsmin ? margin_two : margin_one) - pvs_evss;
			}
		}
		new_depth = depth - pvs_ndse + ext;
		do_move<me>(move);

		if (PrN > 1)
		{
			evaluate();

			if (Current->att[opp] & King(me))
			{
				undo_move<me>(move);
				cnt--;
				goto skip_hash_move;
			}
		}
		value = -pv_search<opp, 0>(-beta, -alpha, new_depth, ExtFlag(ext));
		undo_move<me>(move);
		played++;

		if (value > alpha)
		{
			if (root)
			{
				CurrentSI->fail_low = 0;
				best_move = move;
				best_score = value;
				hash_low(best_move, value, depth);
				send_pv(depth >> 1, old_alpha, beta, value);
			}
			alpha = value;
			Current->best = move;

			if (value >= beta)
				goto cut;
		}
		else if (root)
		{
			CurrentSI->fail_low = 1;
			CurrentSI->fail_high = 0;
			CurrentSI->singular = 0;
			send_pv(depth >> 1, old_alpha, beta, value);
		}
	}
skip_hash_move:
	Current->gen_flags = 0;

	if (F(Check(me)))
	{
		Current->stage = stage_search;
		Current->ref[0] = RefM(Current->move).ref[0];
		Current->ref[1] = RefM(Current->move).ref[1];
	}
	else
	{
		Current->stage = stage_evasion;
		Current->ref[0] = RefM(Current->move).check_ref[0];
		Current->ref[1] = RefM(Current->move).check_ref[1];
	}
	Current->killer[0] = 0;
	Current->moves[0] = 0;

	if (root)
		Current->current = RootList + 1;
	else
		Current->current = Current->moves;

	if (PrN > 1 && !root && parent && depth >= SplitDepthPV)
		do_split = 1;

	while (move = get_move<me, root>())
	{
		if (move == hash_move)
			continue;

		if (IsIllegal(me, move))
			continue;
		cnt++;

		if (root)
		{
			memset(Data + 1, 0, 127 * sizeof(GData));
			move_to_string(move, score_string);
			if (VerboseUCI)
				sprintf(info_string, "info currmove %s currmovenumber %d\n", score_string, cnt);
		}

		if (IsRepetition(alpha + 1, move))
			continue;
		check = is_check<me>(move);

		if (check)
			ext = pvs_ce;
		else
			ext = MaxF(pext, extension<1>(move, depth));
		new_depth = depth - pvs_ndse + ext;

		if (depth >= pvs_rdm && F(move & 0xE000) && F(Square(To(move)))
			&& (T(root) || (move != Current->killer[1] && move != Current->killer[2]) || T(Check(me))) && cnt > pvs_cm)
		{
			int reduction = msb(cnt) - pvs_rs;

			if (move == Current->ref[0] || move == Current->ref[1])
				reduction = Max(0, reduction - pvs_rs2);

			if (reduction >= pvs_rmin && !(Queen(White) | Queen(Black)) && popcnt(NonPawnKingAll) <= 4)
				reduction += reduction >> 1;
			new_depth = Max(pvs_ndmax, new_depth - reduction);
		}

		if (do_split && played >= 1)
		{
			if (!sp_init)
			{
				sp_init = 1;
				uint64 u = ~Smpi->active_sp;

				if (!u)
				{
					do_split = 0;
					goto make_move;
				}
				Sp = &Smpi->sp[lsb(u)];
				init_sp(Sp, alpha, beta, depth, 1, singular, height);
			}
			GMove* M = &Sp->move[Sp->move_number++];
			M->ext = ext;
			M->flags = 0;
			M->move = move;
			M->reduced_depth = new_depth;
			M->research_depth = depth - pvs_Mrds + ext;
			M->stage = Current->stage;
			continue;
		}
	make_move:
		do_move<me>(move);

		if (new_depth <= pvs_npvs)
			value = -pv_search<opp, 0>(-beta, -alpha, new_depth, ExtFlag(ext));
		else
			value = -search<opp, 0>(-alpha, new_depth, FlagNeatSearch | ExtFlag(ext));

		if (value > alpha && new_depth > 1)
		{
			if (root)
			{
				SetScore(RootList[cnt - 1], 1);
				CurrentSI->early = 0;
				old_best = best_move;
				best_move = move;
			}
			new_depth = depth - pvs_ndse + ext;
			value = -pv_search<opp, 0>(-beta, -alpha, new_depth, ExtFlag(ext));

			if (T(root) && value <= alpha)
				best_move = old_best;
		}
		undo_move<me>(move);
		played++;

		if (value > alpha)
		{
			if (root)
			{
				SetScore(RootList[cnt - 1], cnt + 3);
				CurrentSI->change = 1;
				CurrentSI->fail_low = 0;
				best_move = move;
				best_score = value;
				hash_low(best_move, value, depth);
				send_pv(depth >> 1, old_alpha, beta, value);
			}
			alpha = value;
			Current->best = move;

			if (value >= beta)
				goto cut;
		}
	}

	if (do_split && sp_init)
	{
		value = smp_search<me>(Sp);

		if (value > alpha && Sp->best_move)
		{
			alpha = value;
			Current->best = move = Sp->best_move;
		}

		if (value >= beta)
			goto cut;
	}

	if (F(cnt) && F(Check(me)))
	{
		hash_high(0, 127);
		hash_low(0, 0, 127);
		hash_exact(0, 0, 127, 0, 0, 0);
		return 0;
	}

	if (F(root) || F(SearchMoves))
		hash_high(alpha, depth);

	if (alpha > old_alpha)
	{
		hash_low(Current->best, alpha, depth);

		if (Current->best != hash_move)
			ex_depth = 0;

		if (F(root) || F(SearchMoves))
			hash_exact(Current->best, alpha, depth, ex_value, ex_depth, Convert(nodes >> 10, int) - start_knodes);
	}
	return alpha;
cut:
	hash_low(move, alpha, depth);
	return alpha;
}

template <int me> static void root()
{
	int i = 0, depth = 0, value = 0, alpha = 0, beta = 0, delta = 0, hash_depth = 0, hash_value, store_time = 0;
	int time_est = 0, ex_depth = 0, ex_value = 0, prev_time = 0, knodes = 0;
	int start_depth = 2;
	sint64 time = 0;
	GPVEntry* PVEntry = NULL;

	date++;
	nodes = check_node = check_node_smp = 0;

	if (parent)
		Smpi->nodes = Smpi->tb_hits = 0;
	memcpy(Data, Current, sizeof(GData));
	Current = Data;
	evaluate();
	gen_root_moves<me>();

	if (PVN > 1)
	{
		memset(MultiPV, 0, 128 * sizeof(int));

		for (i = 0; MultiPV[i] = RootList[i]; i++);
	}

	best_move = RootList[0];

	if (F(best_move))
		return;

	if (F(Infinite) && !RootList[1])
	{
		Infinite = 1;
		value = pv_search<me, 1>(-MateValue, MateValue, 4, FlagNeatSearch);
		Infinite = 0;
		LastDepth = 128;
		send_pv(6, -MateValue, MateValue, value);
		send_best_move();
		Searching = 0;

		ZERO_BIT_64(Smpi->searching, 0);
		return;
	}

	memset(CurrentSI, 0, sizeof(GSearchInfo));
	memset(BaseSI, 0, sizeof(GSearchInfo));
	Previous = -MateValue;

	if (PVEntry = probe_pv_hash())
	{
		if (is_legal<me>(PVEntry->move) && PVEntry->move == best_move && PVEntry->depth > hash_depth)
		{
			hash_depth = PVEntry->depth;
			hash_value = PVEntry->value;
			ex_depth = PVEntry->ex_depth;
			ex_value = PVEntry->exclusion;
			knodes = PVEntry->knodes;
		}
	}

	if (T(hash_depth) && PVN == 1)
	{
		Previous = best_score = hash_value;
		depth = hash_depth;

		if (PVHashing)
		{
			send_pv(depth >> 1, -MateValue, MateValue, best_score);
			start_depth = (depth + 2) & (~1);
		}

		if ((depth >= LastDepth - r_lds || T(store_time)) && LastValue >= LastExactValue && hash_value >= LastExactValue
			&& T(LastTime) && T(LastSpeed))
		{
			time = TimeLimit1;

			if (ex_depth >= depth - Min(r_edms, depth >> 1) && ex_value <= hash_value - ExclSinglePV(depth))
			{
				BaseSI->early = 1;
				BaseSI->singular = 1;

				if (ex_value <= hash_value - ExclDoublePV(depth))
				{
					time = (time * TimeSingTwoMargin) / 100;
					BaseSI->singular = 2;
				}
				else
					time = (time * TimeSingOneMargin) / 100;
			}
			time_est = Min(LastTime, (knodes << 10) / int(LastSpeed));
			time_est = Max(time_est, store_time);
		set_prev_time:
			LastTime = prev_time = time_est;

			if (prev_time >= time && F(Infinite))
			{
				InstCnt++;

				if (time_est <= store_time)
					InstCnt = 0;

				if (InstCnt > 2)
				{
					if (T(store_time) && store_time < time_est)
					{
						time_est = store_time;
						goto set_prev_time;
					}
					LastSpeed = 0;
					LastTime = 0;
					prev_time = 0;
					goto set_jump;
				}

				if (hash_value > 0 && Current->ply >= 2 && F(Square(To(best_move))) && F(best_move & 0xF000)
					&& PrevMove == ((To(best_move) << 6) | From(best_move)))
					goto set_jump;
				do_move<me>(best_move);

				if (Current->ply >= 100)
				{
					undo_move<me>(best_move);
					goto set_jump;
				}

				for (i = 4; i <= Current->ply; i += 2)
					if (Stack[sp - i] == Current->key)
					{
						undo_move<me>(best_move);
						goto set_jump;
					}
				undo_move<me>(best_move);
				LastDepth = depth;
				LastTime = prev_time;
				LastValue = LastExactValue = hash_value;
				send_best_move();
				Searching = 0;

				if (MaxPrN > 1)
					ZERO_BIT_64(Smpi->searching, 0);
				return;
			}
			else
				goto set_jump;
		}
	}
	LastTime = 0;
set_jump:
	memcpy(SaveBoard, Board, sizeof(GBoard));
	memcpy(SaveData, Data, sizeof(GData));
	save_sp = sp;

	if (setjmp(Jump))
	{
		Current = Data;
		Searching = 0;
		halt_all(0, 127);
		ZERO_BIT_64(Smpi->searching, 0);
		memcpy(Board, SaveBoard, sizeof(GBoard));
		memcpy(Data, SaveData, sizeof(GData));
		sp = save_sp;
		send_best_move();
		return;
	}

	for (depth = start_depth; depth < DepthLimit; depth += 2)
	{
		memset(Data + 1, 0, 127 * sizeof(GData));
		CurrentSI->early = 1;
		CurrentSI->change = CurrentSI->fail_high = CurrentSI->fail_low = CurrentSI->singular = 0;

		if (PVN > 1)
			value = multipv<me>(depth);

		else if ((depth >> 1) < 7 || F(Aspiration))
			LastValue = LastExactValue = value = pv_search<me, 1>(-MateValue, MateValue, depth, FlagNeatSearch);

		else
		{
			delta = r_delta;
			alpha = Previous - delta;
			beta = Previous + delta;
		loop:
			if (delta >= r_dmin)
			{
				LastValue = LastExactValue = value = pv_search<me, 1>(-MateValue, MateValue, depth, FlagNeatSearch);
				goto finish;
			}
			value = pv_search<me, 1>(alpha, beta, depth, FlagNeatSearch);

			if (value <= alpha)
			{
				CurrentSI->fail_high = 0;
				CurrentSI->fail_low = 1;
				alpha -= delta;
				delta *= 2;
				LastValue = value;
				memcpy(BaseSI, CurrentSI, sizeof(GSearchInfo));
				goto loop;
			}
			else if (value >= beta)
			{
				CurrentSI->fail_high = 1;
				CurrentSI->fail_low = 0;
				CurrentSI->early = 1;
				CurrentSI->change = 0;
				CurrentSI->singular = Max(CurrentSI->singular, 1);
				beta += delta;
				delta *= 2;
				LastDepth = depth;
				LastTime = MaxF(prev_time, int(get_time() - StartTime));
				LastSpeed = nodes / Max(LastTime, 1);

				if (depth + 2 < DepthLimit)
					depth += 2;
				InstCnt = 0;
				check_time(LastTime, 0);
				memset(Data + 1, 0, 127 * sizeof(GData));
				LastValue = value;
				memcpy(BaseSI, CurrentSI, sizeof(GSearchInfo));
				goto loop;
			}
			else
				LastValue = LastExactValue = value;
		}
	finish:
		if (value < Previous - r_pvs)
			CurrentSI->bad = 1;
		else
			CurrentSI->bad = 0;
		memcpy(BaseSI, CurrentSI, sizeof(GSearchInfo));
		LastDepth = depth;
		LastTime = MaxF(prev_time, int(get_time() - StartTime));
		LastSpeed = nodes / Max(LastTime, 1);
		Previous = value;
		InstCnt = 0;
		check_time(LastTime, 0);
	}
	Searching = 0;

	ZERO_BIT_64(Smpi->searching, 0);

	if (F(Infinite) || DepthLimit < 128)
		send_best_move();
}

template <int me> static int multipv(int depth)
{
	int move = 0, value = 0, i = 0, cnt = 0, ext = 0;
	int low = MateValue;
	int new_depth = depth;

	for (cnt = 0; cnt < PVN && T(move = (MultiPV[cnt] & 0xFFFF)); cnt++)
	{
		MultiPV[cnt] = move;
		move_to_string(move, score_string);
		if (VerboseUCI)
			sprintf(info_string, "info currmove %s currmovenumber %d\n", score_string, cnt + 1);

		new_depth = depth - 2 + (ext = extension<1>(move, depth));
		do_move<me>(move);
		value = -pv_search<opp, 0>(-MateValue, MateValue, new_depth, ExtFlag(ext));
		MultiPV[cnt] |= value << 16;

		if (value < low)
			low = value;
		undo_move<me>(move);

		for (i = cnt - 1; i >= 0; i--)
		{
			if ((MultiPV[i] >> 16) < value)
			{
				MultiPV[i + 1] = MultiPV[i];
				MultiPV[i] = move | (value << 16);
			}
		}
		best_move = MultiPV[0] & 0xFFFF;
		Current->score = MultiPV[0] >> 16;
		send_multipv((depth >> 1), cnt);
	}

	for (; T(move = (MultiPV[cnt] & 0xFFFF)); cnt++)
	{
		MultiPV[cnt] = move;
		move_to_string(move, score_string);
		if (VerboseUCI)
			sprintf(info_string, "info currmove %s currmovenumber %d\n", score_string, cnt + 1);

		new_depth = depth - 2 + (ext = extension<1>(move, depth));
		do_move<me>(move);
		value = -search<opp, 0>(-low, new_depth, FlagNeatSearch | ExtFlag(ext));

		if (value > low)
			value = -pv_search<opp, 0>(-MateValue, -low, new_depth, ExtFlag(ext));
		MultiPV[cnt] |= value << 16;
		undo_move<me>(move);
		if (PVN < 1)
			PVN = 1;
		if (value > low)
		{
			for (i = cnt; i >= PVN; i--)
				MultiPV[i] = MultiPV[i - 1];
			MultiPV[PVN - 1] = move | (value << 16);

			for (i = PVN - 2; i >= 0; i--)
			{
				if ((MultiPV[i] >> 16) < value)
				{
					MultiPV[i + 1] = MultiPV[i];
					MultiPV[i] = move | (value << 16);
				}
			}
			best_move = MultiPV[0] & 0xFFFF;
			Current->score = MultiPV[0] >> 16;
			low = MultiPV[PVN - 1] >> 16;
			send_multipv((depth >> 1), cnt);
		}
	}
	return Current->score;
}


template <int me> static int draw_in_pv()
{
	if ((Current - Data) >= 126)
		return 1;

	if (Current->ply >= 100)
		return 1;

	for (int i = 4; i <= Current->ply; i += 2)
		if (Stack[sp - i] == Current->key)
			return 1;

	if (GPVEntry*PVEntry = probe_pv_hash())
	{
		if (!PVEntry->value)
			return 1;

		if (int move = PVEntry->move)
		{
			do_move<me>(move);
			int value = draw_in_pv<opp>();
			undo_move<me>(move);
			return value;
		}
	}
	return 0;
}

template <bool pv> static __forceinline int extension(int move, int depth)
{
	register int ext = 0;

	if (pv)
	{
		if (T(Current->passer & Bit(From(move))) && CRank(Current->turn, From(move)) >= 5 && depth < 16)
			ext = 2;
	}
	else
	{
		if (T(Current->passer & Bit(From(move))) && CRank(Current->turn, From(move)) >= 5 && depth < 16)
			ext = 1;
	}
	return ext;
}

template <int me> static void gen_next_moves()
{
	int* p = NULL, *q = NULL, *r = NULL;
	Current->gen_flags &= ~FlagSort;

	switch (Current->stage)
	{
	case s_hash_move:
	case r_hash_move:
	case e_hash_move:
		Current->moves[0] = Current->killer[0];
		Current->moves[1] = 0;
		return;

	case s_good_cap:
		Current->mask = Piece(opp);
		r = gen_captures <me, 0>(Current->moves);

		for (q = r - 1, p = Current->moves; q >= p;)
		{
			int move = (*q) & 0xFFFF;

			if (!see<me>(move, 0))
			{
				int next = *p;
				*p = *q;
				*q = next;
				p++;
			}
			else
				q--;
		}
		Current->start = p;
		Current->current = p;
		sort(p, r);
		return;

	case s_special:
		Current->current = Current->start;
		p = Current->start;

		if (Current->killer[1])
		{
			*p = Current->killer[1];
			p++;
		}

		if (Current->killer[2])
		{
			*p = Current->killer[2];
			p++;
		}

		if (Current->ref[0] && Current->ref[0] != Current->killer[1] && Current->ref[0] != Current->killer[2])
		{
			*p = Current->ref[0];
			p++;
		}

		if (Current->ref[1] && Current->ref[1] != Current->killer[1] && Current->ref[1] != Current->killer[2])
		{
			*p = Current->ref[1];
			p++;
		}
		*p = 0;
		return;

	case s_quiet:
		gen_quiet_moves<me>(Current->start);
		Current->current = Current->start;
		Current->gen_flags |= FlagSort;
		return;

	case s_bad_cap:
		*(Current->start) = 0;
		Current->current = Current->moves;

		if (!(Current->gen_flags & FlagNoBcSort))
			sort(Current->moves, Current->start);
		return;

	case r_cap:
		r = gen_captures <me, 0>(Current->moves);
		Current->current = Current->moves;
		sort(Current->moves, r);
		return;

	case r_checks:
		r = gen_checks<me>(Current->moves);
		Current->current = Current->moves;
		sort(Current->moves, r);
		return;

	case e_ev:
		Current->mask = Filled;
		r = gen_evasions<me>(Current->moves);
		mark_evasions(Current->moves);
		sort(Current->moves, r);
		Current->current = Current->moves;
		return;
	}
}

template <int me, bool root> static int get_move()
{
	int move = 0;

	if (root)
	{
		move = (*Current->current) & 0xFFFF;
		Current->current++;
		return move;
	}
start:
	if (F(*Current->current))
	{
		Current->stage++;

		if ((1 << Current->stage) & StageNone)
			return 0;

		gen_next_moves<me>();
		goto start;
	}

	if (Current->gen_flags & FlagSort)
		move = pick_move();
	else
	{
		move = (*Current->current) & 0xFFFF;
		Current->current++;
	}

	if (Current->stage == s_quiet)
	{
		if (move == Current->killer[1] || move == Current->killer[2] || move == Current->ref[0]
			|| move == Current->ref[1])
			goto start;
	}

	else if (Current->stage == s_special && (Square(To(move)) || !is_legal<me>(move)))
		goto start;
	return move;
}

template <int me> int is_legal(int move)
{
	uint64 u = 0, occ = 0;
	int from = From(move);
	int to = To(move);
	int piece = Board->square[from];
	int capture = Board->square[to];

	if (piece == 0)
		return 0;

	if ((piece & 1) != Current->turn)
		return 0;

	if (capture)
	{
		if ((capture & 1) == (piece & 1))
			return 0;

		if (capture >= WhiteKing)
			return 0;
	}
	occ = PieceAll;
	u = Bit(to);

	if (piece >= WhiteLight && piece < WhiteKing)
	{
		if ((QMask[from] & u) == 0)
			return 0;

		if (Between[from][to] & occ)
			return 0;
	}

	if (IsEP(move))
	{
		if (piece >= WhiteKnight)
			return 0;

		if (Current->ep_square != to)
			return 0;
		return 1;
	}

	if (IsCastling(move) && Board->square[from] < WhiteKing)
		return 0;

	if (IsPromotion(move) && Board->square[from] >= WhiteKnight)
		return 0;

	if (piece == IPawn(me))
	{
		if (u & PMove[me][from])
		{
			if (capture)
				return 0;

			if (T(u & Line(me, 7)) && !IsPromotion(move))
				return 0;
			return 1;
		}
		else if (to == (from + 2 * Push(me)))
		{
			if (capture)
				return 0;

			if (Square(to - Push(me)))
				return 0;

			if (F(u & Line(me, 3)))
				return 0;
			return 1;
		}
		else if (u & PAtt[me][from])
		{
			if (capture == 0)
				return 0;

			if (T(u & Line(me, 7)) && !IsPromotion(move))
				return 0;
			return 1;
		}
		else
			return 0;
	}
	else if (piece == IKing(me))
	{
		if (me == White)
		{
			if (IsCastling(move))
			{
				if (u & 0x40)
				{
					if (((Current->castle_flags) &CanCastle_OO) == 0)
						return 0;

					if (occ & 0x60)
						return 0;

					if (Current->att[Black] & 0x70)
						return 0;
				}
				else
				{
					if (((Current->castle_flags) &CanCastle_OOO) == 0)
						return 0;

					if (occ & 0xE)
						return 0;

					if (Current->att[Black] & 0x1C)
						return 0;
				}
				return 1;
			}
		}
		else
		{
			if (IsCastling(move))
			{
				if (u & 0x4000000000000000)
				{
					if (((Current->castle_flags) &CanCastle_oo) == 0)
						return 0;

					if (occ & 0x6000000000000000)
						return 0;

					if (Current->att[White] & 0x7000000000000000)
						return 0;
				}
				else
				{
					if (((Current->castle_flags) &CanCastle_ooo) == 0)
						return 0;

					if (occ & 0x0E00000000000000)
						return 0;

					if (Current->att[White] & 0x1C00000000000000)
						return 0;
				}
				return 1;
			}
		}

		if (F(SArea[from] & u))
			return 0;

		if (Current->att[opp] & u)
			return 0;
		return 1;
	}
	piece = (piece >> 1) - 2;

	if (piece == 0)
	{
		if (u & NAtt[from])
			return 1;
		else
			return 0;
	}
	else
	{
		if (piece <= 2)
		{
			if (BMask[from] & u)
				return 1;
		}
		else if (piece == 3)
		{
			if (RMask[from] & u)
				return 1;
		}
		else
			return 1;
		return 0;
	}
}

template <int me> static int is_check(int move)
{
	int from = From(move);
	int to = To(move);
	uint64 king = King(opp);
	int king_sq = lsb(king);
	int piece = Square(from);

	if (T(Bit(from) & Current->xray[me]) && F(FullLine[king_sq][from] & Bit(to)))
		return 1;

	if (piece < WhiteKnight)
	{
		if (PAtt[me][to] & king)
			return 1;

		if (T(Bit(to) & Line(me, 7)) && T(king & Line(me, 7)) && F(Between[to][king_sq] & PieceAll))
			return 1;
	}
	else if (piece < WhiteLight)
	{
		if (NAtt[to] & king)
			return 1;
	}
	else if (piece < WhiteRook)
	{
		if (BMask[to] & king)
			if (F(Between[king_sq][to] & PieceAll))
				return 1;
	}
	else if (piece < WhiteQueen)
	{
		if (RMask[to] & king)
			if (F(Between[king_sq][to] & PieceAll))
				return 1;
	}
	else if (piece < WhiteKing)
	{
		if (QMask[to] & king)
			if (F(Between[king_sq][to] & PieceAll))
				return 1;
	}
	return 0;
}

template <int me> static int singular_extension(int ext, int prev_ext, int margin_one, int margin_two, int depth, int killer)
{
	int value = -MateValue;
	int singular = 0;

	if (ext < 1 + (prev_ext < 1))
	{
		if (Check(me))
			value = search_evasion<me, 1>(margin_one, depth, killer);
		else
			value = search<me, 1>(margin_one, depth, killer);

		if (value < margin_one)
			singular = 1;
	}

	if (value < margin_one && ext < 2 + (prev_ext < 1) - (prev_ext >= 2))
	{
		if (Check(me))
			value = search_evasion<me, 1>(margin_two, depth, killer);
		else
			value = search<me, 1>(margin_two, depth, killer);

		if (value < margin_two)
			singular = 2;
	}
	return singular;
}

template <int me> static __forceinline void capture_margin(int alpha, int &score)
{
	if (Current->score + cm_pawn < alpha)
	{
		if (Current->att[me] & Pawn(opp))
		{
			Current->mask ^= Pawn(opp);
			score = Current->score + cm_pawn;
		}

		if (Current->score + cm_minor < alpha)
		{
			if (Current->att[me] & Minor(opp))
			{
				Current->mask ^= Minor(opp);
				score = Current->score + cm_minor;
			}

			if (Current->score + cm_rook < alpha)
			{
				if (Current->att[me] & Rook(opp))
				{
					Current->mask ^= Rook(opp);
					score = Current->score + cm_rook;
				}

				if (Current->score + cm_queen < alpha && (Current->att[me] & Queen(opp)))
				{
					Current->mask ^= Queen(opp);
					score = Current->score + cm_queen;
				}
			}
		}
	}
}

#endif