#ifndef SMP_H_INCLUDED
#define SMP_H_INCLUDED

template <int me> static int smp_search(GSP* Sp)
{
	int i = 0, value = 0, move = 0, alpha = 0, iter = 0;

	if (!Sp->move_number)
		return Sp->alpha;

	send_position(Sp->position);

	if (setjmp(Sp->jump))
	{
		LOCK(Sp->lock);
		halt_all(Sp, 1);
		UNLOCK(Sp->lock);
		halt_all(Sp->height + 1, 127);
		Current = Data + Sp->height;
		sp = Sp->position->sp;
		retrieve_board(Sp->position);
		return Sp->beta;
	}
	LOCK(Sp->lock);
	SET_BIT_64(Smpi->active_sp, (int)(Sp - Smpi->sp));
	Sp->active = 1;
	Sp->claimed = Sp->finished = 0;
loop:
	for (i = 0; i < Sp->move_number; i++)
	{
		GMove* M = &Sp->move[i];

		if (!iter)
			Sp->current = i;

		if (M->flags & FlagFinished)
			continue;

		if (!iter)
		{
			if (M->flags & FlagClaimed)
				continue;
			M->flags |= FlagClaimed;
			M->id = Id;
		}
		else if (M->flags & FlagClaimed)
		{
			SET_BIT_64(Smpi->stop, M->id);
			M->id = Id;
		}
		move = M->move;
		alpha = Sp->alpha;
		UNLOCK(Sp->lock);
		do_move<me>(move);
		value = -search<opp, 0>(-alpha, M->reduced_depth, FlagNeatSearch | ExtFlag(M->ext));

		if (value > alpha && (Sp->pv || M->reduced_depth < M->research_depth))
		{
			if (Sp->pv)
				value = -pv_search<opp, 0>(-Sp->beta, -Sp->alpha, M->research_depth, FlagNeatSearch | ExtFlag(M->ext));
			else
				value = -search<opp, 0>(-alpha, M->research_depth, FlagNeatSearch | FlagDisableNull | ExtFlag(M->ext));
		}
		undo_move<me>(move);
		LOCK(Sp->lock);

		if (Sp->finished)
			goto cut;
		M->flags |= FlagFinished;

		if (value > Sp->alpha)
		{
			Sp->best_move = move;
			Sp->alpha = Min(value, Sp->beta);

			if (value >= Sp->beta)
				goto cut;
		}
	}

	if (!iter)
	{
		iter++;
		goto loop;
	}
	halt_all(Sp, 1);
	UNLOCK(Sp->lock);
	return Sp->alpha;
cut:
	halt_all(Sp, 1);
	UNLOCK(Sp->lock);
	return Sp->beta;
}

#endif