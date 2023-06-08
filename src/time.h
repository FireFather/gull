#ifndef TIME_H_INCLUDED
#define TIME_H_INCLUDED

void get_time_limit(char string[])
{
	const char* ptr = strtok(string, " ");
	int i = 0, time = 0, inc = 0;
	int wtime = 0, btime = 0;
	int winc = 0, binc = 0;
	int moves = 0, pondering = 0;
	int movetime = 0;
	int exp_moves = MovesTg - 1;
	Infinite = 1;
	MoveTime = 0;
	SearchMoves = 0;
	SMPointer = 0;
	TimeLimit1 = 0;
	TimeLimit2 = 0;
	Stop = 0;
	DepthLimit = 128;

	for (ptr = strtok(NULL, " "); ptr != NULL; ptr = strtok(NULL, " "))
	{
		if (!strcmp(ptr, "binc"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				binc = atoi(ptr);
			Infinite = 0;
		}
		else if (!strcmp(ptr, "btime"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				btime = atoi(ptr);
			Infinite = 0;
		}
		else if (!strcmp(ptr, "depth"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				DepthLimit = 2 * atoi(ptr) + 2;
			Infinite = 1;
		}
		else if (!strcmp(ptr, "infinite"))
		{
			Infinite = 1;
		}
		else if (!strcmp(ptr, "movestogo"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				moves = atoi(ptr);
			Infinite = 0;
		}
		else if (!strcmp(ptr, "winc"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				winc = atoi(ptr);
			Infinite = 0;
		}
		else if (!strcmp(ptr, "wtime"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				wtime = atoi(ptr);
			Infinite = 0;
		}
		else if (!strcmp(ptr, "movetime"))
		{
			ptr = strtok(NULL, " ");
			if (ptr)
				movetime = atoi(ptr);
			MoveTime = 1;
			Infinite = 0;
		}
		else if (!strcmp(ptr, "searchmoves"))
		{
			if (F(SearchMoves))
			{
				for (i = 0; i < 256; i++)
					SMoves[i] = 0;
			}
			SearchMoves = 1;
			ptr += 12;

			while (ptr != NULL && ptr[0] >= 'a' && ptr[0] <= 'h' && ptr[1] >= '1' && ptr[1] <= '8')
			{
				pv_string[0] = *ptr++;
				pv_string[1] = *ptr++;
				pv_string[2] = *ptr++;
				pv_string[3] = *ptr++;

				if (*ptr == 0 || *ptr == ' ')
					pv_string[4] = 0;
				else
				{
					pv_string[4] = *ptr++;
					pv_string[5] = 0;
				}
				SMoves[SMPointer] = move_from_string(pv_string);
				SMPointer++;
				ptr = strtok(NULL, " ");
			}
		}

		else if (!strcmp(ptr, "ponder"))
			pondering = 1;
	}

	if (BenchMarking)
	{
		movetime = 5000;
		MoveTime = 1;
		Infinite = 0;
	}

	if (pondering)
		Infinite = 1;
	if (Current->turn == White)
	{
		time = wtime;
		inc = winc;
	}
	else
	{
		time = btime;
		inc = binc;
	}

	if (moves)
	{
		moves = Max(moves - 1, 1);
	}
	int time_max = Max(time - Min(1000, time >> 1), 0);
	int nmoves;

	if (moves)
		nmoves = moves;
	else
	{
		nmoves = MovesTg - 1;

		if (Current->ply > 40)
			nmoves += Min(Current->ply - 40, (100 - Current->ply) >> 1);
		exp_moves = nmoves;
	}
	TimeLimit1 = Min(time_max, (time_max + (Min(exp_moves, nmoves) * inc)) / Min(exp_moves, nmoves));
	TimeLimit2 = Min(time_max, (time_max + (Min(exp_moves, nmoves) * inc)) / Min(3, Min(exp_moves, nmoves)));
	TimeLimit1 = Min(time_max, (TimeLimit1 * TimeRatio) / 100);

	if (Ponder)
		TimeLimit1 = (TimeLimit1 * PonderRatio) / 100;

	if (MoveTime)
	{
		TimeLimit2 = movetime;
		TimeLimit1 = TimeLimit2 * 100;
	}
	InfoTime = StartTime = get_time();
	Searching = 1;

	SET_BIT_64(Smpi->searching, 0);

	if (F(Infinite))
		PVN = 1;

	if (Current->turn == White)
		root<0>();
	else
		root<1>();
}

#endif