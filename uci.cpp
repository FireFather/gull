#include <windows.h>
#include <iostream>
#include "def.h"
#include "macro.h"
#include "struct.h"
#include "function.h"
#include "seagull.h"

char mstring[65536];
int ResetHash = 1;
jmp_buf ResetJump;
int NewPrN = 0;
int Ponder;
int PVHashing = 1;
int Aspiration = 1;
int VerboseUCI = 0;
int TBProbeDepth = 8;
int TBProbeLimit = 6;
int PrN = 1;
int CPUs = 1;
int LargePages = 1;
static char TBPath[1024];
int SMPointer;
uint16 SMoves[256];

void uci()
{
	char* ptr = NULL;
	int i = 0;
	int value = 0;

	(void)fgets(mstring, 65536, stdin);

	if (feof(stdin))
		exit(0);
	ptr = strchr(mstring, '\n');

	if (ptr != NULL)
		*ptr = 0;

	if (!strcmp(mstring, "uci"))
	{
		fprintf(stdout, "id name " ENGINE " " VERSION " " PLATFORM "\n");
		fprintf(stdout, "id author " AUTHOR "\n");

#ifndef W32_BUILD
		fprintf(stdout, "option name Hash type spin min 1 max 65536 default 128\n");
#else
		fprintf(stdout, "option name Hash type spin min 1 max 1024 default 32\n");
#endif

		fprintf(stdout, "option name Threads type spin min 1 max %d default %d\n", Min(CPUs, MaxPrN), PrN);
		fprintf(stdout, "option name MultiPV type spin min 1 max 64 default 1\n");
		fprintf(stdout, "option name Ponder type check default false\n");
		fprintf(stdout, "option name VerboseUCI type check default false\n");
		fprintf(stdout, "option name PV Hash type check default true\n");
		fprintf(stdout, "option name Aspiration window type check default true\n");
		fprintf(stdout, "option name Large memory pages type check default true\n");
		fprintf(stdout, "option name TBPath type string default <empty>\n");
		fprintf(stdout, "option name TBProbeDepth type spin min 0 max 64 default 8\n");
		fprintf(stdout, "option name TBProbeLimit type spin min 0 max 6 default 6\n");
		fprintf(stdout, "option name Clear Hash type button\n");
		fprintf(stdout, "uciok\n");

		if (F(Searching))
			init_search(1);
	}
	else if (!strcmp(mstring, "ucinewgame"))
	{
		Stop = 0;
		init_search(1);
	}
	else if (!strcmp(mstring, "isready"))
	{
		fprintf(stdout, "readyok\n");
		fflush(stdout);
	}
	else if (!memcmp(mstring, "position", 8))
	{
		if (F(Searching))
			get_position(mstring);
	}
	else if (!memcmp(mstring, "go", 2))
	{
		if (F(Searching))
			get_time_limit(mstring);
	}
	else if (!memcmp(mstring, "setoption", 9))
	{
		ptr = strtok(mstring, " ");

		for (ptr = strtok(NULL, " "); ptr != NULL; ptr = strtok(NULL, " "))
		{
			if (!memcmp(ptr, "Hash", 4) && !Searching)
			{
				ptr += 11;
				value = atoi(ptr);

				if (value < 1)
					value = 1;

#ifdef W32_BUILD
				if (value > 1024)
					value = 1024;
#else
				if (value > 65536)
					value = 65536;
#endif

				value = (Bit(msb(value)) * Convert(1024 * 1024, sint64)) / Convert(sizeof(GEntry), sint64);

				if (value != hash_size)
				{
					ResetHash = 1;
					hash_size = value;
					longjmp(ResetJump, 1);
				}
			}
			else if (!memcmp(ptr, "Threads", 7) && !Searching)
			{
				ptr += 14;
				value = atoi(ptr);

				if (value != PrN)
				{
					NewPrN = Max(1, Min(MaxPrN, value));
					ResetHash = 0;
					longjmp(ResetJump, 1);
				}
			}
			else if (!memcmp(ptr, "MultiPV", 7))
			{
				ptr += 14;
				PVN = atoi(ptr);
				Stop = 1;
			}
			else if (!memcmp(ptr, "Ponder", 6))
			{
				ptr += 13;

				if (ptr[0] == 't')
					Ponder = 1;
				else
					Ponder = 0;
			}
			else if (!memcmp(ptr, "Clear", 5))
			{
				init_search(1);
				break;
			}
			else if (!memcmp(ptr, "PV", 2))
			{
				ptr += 14;

				if (ptr[0] == 't')
					PVHashing = 1;
				else
					PVHashing = 0;
			}
			else if (!memcmp(ptr, "Large", 5) && !Searching)
			{
				ptr += 25;

				if (ptr[0] == 't')
				{
					if (LargePages)
						return;

					LargePages = 1;
				}
				else
				{
					if (!LargePages)
						return;
					LargePages = 0;
				}
				ResetHash = 1;
				longjmp(ResetJump, 1);
			}
			else if (!memcmp(ptr, "Aspiration", 10))
			{
				ptr += 24;

				if (ptr[0] == 't')
					Aspiration = 1;
				else
					Aspiration = 0;
			}
			else if (!memcmp(ptr, "VerboseUCI", 10))
			{
				ptr += 17;

				if (ptr[0] == 't')
					VerboseUCI = 1;
				else
					VerboseUCI = 0;
			}
			else if (!memcmp(ptr, "TBPath", 6))
			{
				ptr += 13;
				strcpy(TBPath, ptr);
				if (!TBs_initialized)
					Tablebases::init(ptr);
			}
			else if (!memcmp(ptr, "TBProbeDepth", 12))
			{
				ptr += 19;
				TBProbeDepth = atoi(ptr);
			}
			else if (!memcmp(ptr, "TBProbeLimit", 12))
			{
				ptr += 19;
				TBProbeLimit = atoi(ptr);
			}
		}
	}
	else if (!strcmp(mstring, "stop"))
	{
		Stop = 1;

		if (F(Searching))
			send_best_move();
	}
	else if (!strcmp(mstring, "ponderhit"))
	{
		Infinite = 0;

		if (!RootList[1])
			Stop = 1;

		if (F(CurrentSI->bad) && F(CurrentSI->fail_low) && time_to_stop(BaseSI, LastTime, 0))
			Stop = 1;

		if (F(Searching))
			send_best_move();
	}
	else if (!strcmp(mstring, "quit"))
	{
		for (i = 1; i < PrN; i++)
		{
			TerminateProcess(ChildPr[i], 0);
			CloseHandle(ChildPr[i]);
		}
		exit(0);
	}
	else if (!strcmp(mstring, "bench"))
		Bench();
}

void get_position(char string[])
{
	const char* fen = strstr(string, "fen ");
	char* moves = strstr(string, "moves ");
	const char* ptr = NULL;
	int move = 0, move1 = 0;

	if (fen != NULL)
		get_board(fen + 4);
	else
		get_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	PrevMove = 0;

	if (moves != NULL)
	{
		ptr = moves + 6;

		while (*ptr != 0)
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
			evaluate();
			move = move_from_string(pv_string);
			PrevMove = move1;
			move1 = move;

			if (Current->turn)
				do_move <1>(move);
			else
				do_move <0>(move);
			memcpy(Data, Current, sizeof(GData));
			Current = Data;

			while (*ptr == ' ')
				ptr++;
		}
	}
	memcpy(Stack, Stack + sp - Current->ply, (Current->ply + 1) * sizeof(uint64));
	sp = Current->ply;
}
