#include <windows.h>
#include <iostream>
#include <conio.h>
#include "def.h"
#include "macro.h"
#include "struct.h"
#include "function.h"
#include "seagull.h"

uint16 date;
GPVEntry* PVHash = NULL;
int best_move;
GSearchInfo CurrentSI[1];
GSearchInfo BaseSI[1];
int best_score;
int LastTime;
int LastValue;
int LastExactValue;
int InstCnt;
sint64 LastSpeed;
int Infinite;
int SearchMoves;
int TimeLimit1;
int TimeLimit2;
int Stop;
int Searching;
int DepthLimit;
int LastDepth;
uint64 nodes;
GSMPI* Smpi;
sint16 History[16 * 64];
sint16 Delta[16 * 4096];
GRef Ref[16 * 64];
__declspec(align(64)) GData Data[128];
GData* Current = Data;
int pv_length;
int RootList[256];
uint16 PV[128];
int pvp;
char pv_string[1024];
char score_string[16];
sint64 StartTime;
bool BenchMarking = false;
int PVN = 1;
int MultiPV[256];
int MoveTime;
static sint64 CurrTime;
sint64 InfoTime;
char info_string[1024];
jmp_buf Jump;

void init_search(int clear_hash)
{
	memset(History, 1, 16 * 64 * sizeof(sint16));
	memset(Delta, 0, 16 * 4096 * sizeof(sint16));
	memset(Ref, 0, 16 * 64 * sizeof(GRef));
	memset(Data + 1, 0, 127 * sizeof(GData));

	if (clear_hash)
	{
		date = 1;
		memset(Hash, 0, hash_size * sizeof(GEntry));
		memset(PVHash, 0, pv_hash_size * sizeof(GPVEntry));
	}
	get_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	nodes = 0;
	best_move = best_score = 0;
	LastTime = LastValue = LastExactValue = InstCnt = 0;
	LastSpeed = 0;
	Infinite = 1;
	SearchMoves = 0;
	TimeLimit1 = TimeLimit2 = 0;
	Stop = Searching = 0;
	ZERO_BIT_64(Smpi->searching, 0);
	DepthLimit = 128;
	LastDepth = 128;
	memset(CurrentSI, 0, sizeof(GSearchInfo));
	memset(BaseSI, 0, sizeof(GSearchInfo));
}

void sort(int* start, int* finish)
{
	for (int*p = start; p < finish - 1; p++)
	{
		int* best = p;
		int value = *p;
		int previous = *p;

		for (int*q = p + 1; q < finish; q++)
			if ((*q) > value)
			{
				value = *q;
				best = q;
			}
		*best = previous;
		*p = value;
	}
}

void sort_moves(int* start, int* finish)
{
	for (int*p = start + 1; p < finish; p++)
		for (int*q = p - 1; q >= start; q--)
			if (((*q) >> 16) < ((*(q + 1)) >> 16))
			{
				int move = *q;
				*q = *(q + 1);
				*(q + 1) = move;
			}
}

__forceinline int pick_move()
{
	register int move = *(Current->current);
	if (F(move))
		return 0;
	register int * best = Current->current;
	register int * p = NULL;

	for (p = Current->current + 1; T(*p); p++)
	{
		if ((*p) > move)
		{
			best = p;
			move = *p;
		}
	}
	*best = *(Current->current);
	*(Current->current) = move;
	Current->current++;
	return move & 0xFFFF;
}

void pick_pv()
{
	GEntry* Entry = NULL;
	GPVEntry* PVEntry = NULL;
	int i = 0, depth = -256, move = 0;

	if (pvp >= Min(pv_length, 64))
	{
		PV[pvp] = 0;
		return;
	}

	if (Entry = probe_hash())
		if (T(Entry->move) && Entry->low_depth > depth)
		{
			depth = Entry->low_depth;
			move = Entry->move;
		}

	if (PVEntry = probe_pv_hash())
		if (T(PVEntry->move) && PVEntry->depth > depth)
		{
			depth = PVEntry->depth;
			move = PVEntry->move;
		}
	evaluate();

	if (Current->att[Current->turn] & King(Current->turn ^ 1))
		PV[pvp] = 0;
	else if (move && (Current->turn ? is_legal <1>(move) : is_legal <0>(move)))
	{
		PV[pvp] = move;
		pvp++;

		if (Current->turn)
			do_move <1>(move);
		else
			do_move <0>(move);

		if (Current->ply >= 100)
			goto finish;

		for (i = 4; i <= Current->ply; i += 2)
			if (Stack[sp - i] == Current->key)
			{
				PV[pvp] = 0;
				goto finish;
			}
		pick_pv();
	finish:
		if (Current->turn ^ 1)
			undo_move <1>(move);
		else
			undo_move <0>(move);
	}
	else
		PV[pvp] = 0;
}

void send_pv(int depth, int alpha, int beta, int score)
{
	int i = 0, pos = 0, move = 0, mate = 0, mate_score = 0, sel_depth = 0;
	sint64 time = 0, nps = 0, snodes = 0;

	for (sel_depth = 1; sel_depth < 127 && T((Data + sel_depth)->att[0]); sel_depth++);
	sel_depth--;

	pv_length = 64;

	if (F(move = best_move))
		move = RootList[0];

	if (F(move))
		return;
	PV[0] = move;

	if (Current->turn)
		do_move <1>(move);
	else
		do_move <0>(move);
	pvp = 1;
	pick_pv();

	if (Current->turn ^ 1)
		undo_move <1>(move);
	else
		undo_move <0>(move);
	pos = 0;

	for (i = 0; i < 64 && T(PV[i]); i++)
	{
		if (pos > 0)
		{
			pv_string[pos] = ' ';
			pos++;
		}
		move = PV[i];
		pv_string[pos++] = ((move >> 6) & 7) + 'a';
		pv_string[pos++] = ((move >> 9) & 7) + '1';
		pv_string[pos++] = (move & 7) + 'a';
		pv_string[pos++] = ((move >> 3) & 7) + '1';

		if (IsPromotion(move))
		{
			if ((move & 0xF000) == FlagPQueen)
				pv_string[pos++] = 'q';

			else if ((move & 0xF000) == FlagPRook)
				pv_string[pos++] = 'r';

			else if ((move & 0xF000) == FlagPLight || (move & 0xF000) == FlagPDark)
				pv_string[pos++] = 'b';

			else if ((move & 0xF000) == FlagPKnight)
				pv_string[pos++] = 'n';
		}
		pv_string[pos] = 0;
	}
	score_string[0] = 'c';
	score_string[1] = 'p';

	if (score > MateScore)
	{
		mate = 1;
		strcpy(score_string, "mate ");
		mate_score = (MateValue - score + 1) >> 1;
		score_string[6] = 0;
	}
	else if (score < -MateScore)
	{
		mate = 1;
		strcpy(score_string, "mate ");
		mate_score = -(score + MateValue + 1) >> 1;
		score_string[6] = 0;
	}
	else
	{
		score_string[0] = 'c';
		score_string[1] = 'p';
		score_string[2] = ' ';
		score_string[3] = 0;
	}
	time = get_time() - StartTime;
	snodes = Smpi->nodes;

	if (time != 0)
		nps = (snodes / time) * 1000;

	if (!BenchMarking)
	{
		if (score < beta)
		{
			if (score <= alpha)
				fprintf(stdout, "info depth %d seldepth %d score %s%d upperbound nodes %I64d nps %I64d tbhits %I64d pv %s\n", depth,
				sel_depth, score_string, (mate ? mate_score : score), snodes, nps, Smpi->tb_hits, pv_string);
			else
				fprintf(stdout, "info depth %d seldepth %d score %s%d nodes %I64d nps %I64d tbhits %I64d pv %s\n", depth, sel_depth,
				score_string, (mate ? mate_score : score), snodes, nps, Smpi->tb_hits, pv_string);
		}
		else
			fprintf(stdout, "info depth %d seldepth %d score %s%d lowerbound nodes %I64d nps %I64d tbhits %I64d pv %s\n", depth,
			sel_depth, score_string, (mate ? mate_score : score), snodes, nps, Smpi->tb_hits, pv_string);
		fflush(stdout);

	}
}

void send_multipv(int depth, int curr_number)
{
	int i = 0, j = 0, pos = 0, move = 0, score = 0;
	sint64 time = 0, nps = 0, snodes = 0;

	for (j = 0; j < PVN && T(MultiPV[j]); j++)
	{
		pv_length = 63;
		pvp = 0;
		move = MultiPV[j] & 0xFFFF;
		score = MultiPV[j] >> 16;
		memset(PV, 0, 64 * sizeof(uint16));

		if (Current->turn)
			do_move <1>(move);
		else
			do_move <0>(move);
		pick_pv();

		if (Current->turn ^ 1)
			undo_move <1>(move);
		else
			undo_move <0>(move);

		for (i = 63; i > 0; i--)
			PV[i] = PV[i - 1];
		PV[0] = move;
		pos = 0;

		for (i = 0; i < 64 && T(PV[i]); i++)
		{
			if (pos > 0)
			{
				pv_string[pos] = ' ';
				pos++;
			}
			move = PV[i];
			pv_string[pos++] = ((move >> 6) & 7) + 'a';
			pv_string[pos++] = ((move >> 9) & 7) + '1';
			pv_string[pos++] = (move & 7) + 'a';
			pv_string[pos++] = ((move >> 3) & 7) + '1';

			if (IsPromotion(move))
			{
				if ((move & 0xF000) == FlagPQueen)
					pv_string[pos++] = 'q';

				else if ((move & 0xF000) == FlagPRook)
					pv_string[pos++] = 'r';

				else if ((move & 0xF000) == FlagPLight || (move & 0xF000) == FlagPDark)
					pv_string[pos++] = 'b';

				else if ((move & 0xF000) == FlagPKnight)
					pv_string[pos++] = 'n';
			}
			pv_string[pos] = 0;
		}
		score_string[0] = 'c';
		score_string[1] = 'p';

		if (score > MateScore)
		{
			strcpy(score_string, "mate ");
			score = (MateValue - score + 1) >> 1;
			score_string[6] = 0;
		}
		else if (score < -MateScore)
		{
			strcpy(score_string, "mate ");
			score = -(score + MateValue + 1) >> 1;
			score_string[6] = 0;
		}
		else
		{
			score_string[0] = 'c';
			score_string[1] = 'p';
			score_string[2] = ' ';
			score_string[3] = 0;
		}
		time = get_time() - StartTime;
		snodes = Smpi->nodes;

		if (time != 0)
			nps = (snodes / time) * 1000;

		if (!BenchMarking)
		{
			fprintf(stdout, "info multipv %d depth %d score %s%d nodes %I64d nps %I64d tbhits %I64d pv %s\n", j + 1,
				(j <= curr_number ? depth : depth - 1), score_string, score, snodes, nps, Smpi->tb_hits, pv_string);
			fflush(stdout);
		}
	}
}

void send_best_move()
{
	uint64 snodes = 0;
	int ponder = 0;
	snodes = Smpi->nodes;

	if (VerboseUCI && !BenchMarking)
		fprintf(stdout, "info nodes %I64d score cp %d\n", snodes, best_score);

	if (!best_move)
		return;
	Current = Data;
	evaluate();

	if (Current->turn)
		do_move <1>(best_move);
	else
		do_move <0>(best_move);
	pv_length = 1;
	pvp = 0;
	pick_pv();
	ponder = PV[0];

	if (Current->turn ^ 1)
		undo_move <1>(best_move);
	else
		undo_move <0>(best_move);
	move_to_string(best_move, pv_string);

	if (!BenchMarking)
	{
		if (ponder)
		{
			move_to_string(ponder, score_string);
			fprintf(stdout, "bestmove %s ponder %s\n", pv_string, score_string);
		}
		else
			fprintf(stdout, "bestmove %s\n", pv_string);
		fflush(stdout);
	}
}

void move_to_string(int move, char string[])
{
	int pos = 0;
	string[pos++] = ((move >> 6) & 7) + 'a';
	string[pos++] = ((move >> 9) & 7) + '1';
	string[pos++] = (move & 7) + 'a';
	string[pos++] = ((move >> 3) & 7) + '1';

	if (IsPromotion(move))
	{
		if ((move & 0xF000) == FlagPQueen)
			string[pos++] = 'q';

		else if ((move & 0xF000) == FlagPRook)
			string[pos++] = 'r';

		else if ((move & 0xF000) == FlagPLight || (move & 0xF000) == FlagPDark)
			string[pos++] = 'b';

		else if ((move & 0xF000) == FlagPKnight)
			string[pos++] = 'n';
	}
	string[pos] = 0;
}

int move_from_string(char string[])
{
	int from = ((string[1] - '1') * 8) + (string[0] - 'a');
	int to = ((string[3] - '1') * 8) + (string[2] - 'a');
	int move = (from << 6) | to;

	if (Board->square[from] >= WhiteKing && Abs(to - from) == 2)
		move |= FlagCastling;

	if (T(Current->ep_square) && to == Current->ep_square)
		move |= FlagEP;

	if (string[4] != 0)
	{
		if (string[4] == 'q')
			move |= FlagPQueen;

		else if (string[4] == 'r')
			move |= FlagPRook;
		else if (string[4] == 'b')
		{
			if (Odd(to ^ Rank(to)))
				move |= FlagPLight;
			else
				move |= FlagPDark;
		}

		else if (string[4] == 'n')
			move |= FlagPKnight;
	}
	return move;
}

int time_to_stop(GSearchInfo* SI, int time, int searching)
{
	if (Infinite)
		return 0;

	if (time > TimeLimit2)
		return 1;

	if (searching)
		return 0;

	if (2 * time > TimeLimit2 && F(MoveTime))
		return 1;

	if (SI->bad)
		return 0;

	if (time > TimeLimit1)
		return 1;

	if (T(SI->change) || T(SI->fail_low))
		return 0;

	if (time * 100 > TimeLimit1 * TimeNoChangeMargin)
		return 1;

	if (F(SI->early))
		return 0;

	if (time * 100 > TimeLimit1 * TimeNoPVSCOMargin)
		return 1;

	if (SI->singular < 1)
		return 0;

	if (time * 100 > TimeLimit1 * TimeSingOneMargin)
		return 1;

	if (SI->singular < 2)
		return 0;

	if (time * 100 > TimeLimit1 * TimeSingTwoMargin)
		return 1;
	return 0;
}

sint64 get_time()
{
	return GetTickCount64();
}

void check_time(int searching)
{
	CurrTime = get_time();
	int Time = Convert(CurrTime - StartTime, int);

	while (!Stop && input())
		uci();

	if (Stop)
		goto jump;

	if (Time > InfoLag && CurrTime - InfoTime > InfoDelay)
	{
		InfoTime = CurrTime;

		if (info_string[0])
		{
			fprintf(stdout, "%s", info_string);
			info_string[0] = 0;
			fflush(stdout);
		}
	}

	if (time_to_stop(CurrentSI, Time, searching))
		goto jump;
	return;
jump:
	Stop = 1;
	longjmp(Jump, 1);
}

void check_time(int time, int searching)
{
	CurrTime = get_time();
	int Time = Convert(CurrTime - StartTime, int);

	while (!Stop && input())
		uci();

	if (Stop)
		goto jump;

	if (Time > InfoLag && CurrTime - InfoTime > InfoDelay)
	{
		InfoTime = CurrTime;

		if (info_string[0])
		{
			fprintf(stdout, "%s", info_string);
			info_string[0] = 0;
			fflush(stdout);
		}
	}

	if (time_to_stop(CurrentSI, time, searching))
		goto jump;
	return;
jump:
	Stop = 1;
	longjmp(Jump, 1);
}

int input()
{
	//	if (stdin->cnt > 0) return 1;

	if (child)
		return 0;

	static HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	static BOOL console = GetConsoleMode(hInput, &mode);

	if (!console)
	{
		DWORD total_bytes_avail;
		if (!PeekNamedPipe(hInput, 0, 0, 0, &total_bytes_avail, 0))	return true;
		return total_bytes_avail;
	}
	else return _kbhit();
}