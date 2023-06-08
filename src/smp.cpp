#include <windows.h>
#include <iostream>
#include <intrin.h>

#include "def.h"
#include "macro.h"
#include "struct.h"
#include "function.h"
#include "seagull.h"

int parent = 1;
int child = 0;
int Id = 0;
int WinParId;
int HardwarePopCnt;
int Console;
HANDLE StreamHandle;
HANDLE ChildPr[MaxPrN];
static HANDLE SHARED = NULL;
static jmp_buf CheckJump;

void init_proc()
{
	int i;
reset_jump:
	if (parent)
	{
		if (setjmp(ResetJump))
		{
			for (i = 1; i < PrN; i++)
				TerminateProcess(ChildPr[i], 0);
			for (i = 1; i < PrN; i++)
			{
				WaitForSingleObject(ChildPr[i], INFINITE);
				CloseHandle(ChildPr[i]);
			}
			Smpi->searching = Smpi->active_sp = Smpi->stop = 0;
			for (i = 0; i < MaxSplitPoints; i++)
				Smpi->sp->active = Smpi->sp->claimed = 0;
			Smpi->hash_size = hash_size;
			if (NewPrN)
				Smpi->PrN = PrN = NewPrN;
			goto reset_jump;
		}
		Smpi->hash_size = hash_size;
		Smpi->PrN = PrN;
	}
	else
	{
		hash_size = Smpi->hash_size;
		PrN = Smpi->PrN;
	}
	if (ResetHash)
		init_hash();
	init_search(0);
	if (child)
		while (true)
			check_state();
	if (parent)
		for (i = 1; i < PrN; i++)
			ChildPr[i] = CreateChildProcess(i);
}

void init_sys(int argc, char* argv[])
{
	DWORD p = 0;
	SYSTEM_INFO sysinfo;

	fprintf(stdout, "" ENGINE " " VERSION " " PLATFORM "\n");
	fprintf(stdout, "by " AUTHOR "\n");

	if (argc >= 2)
		if (!memcmp(argv[1], "child", 5))
		{
			child = 1;
			parent = 0;
			WinParId = atoi(argv[2]);
			Id = atoi(argv[3]);
		}

	int CPUInfo[4] = { -1 };
	__cpuid(CPUInfo, 1);
	HardwarePopCnt = (CPUInfo[2] >> 23) & 1;

	if (HardwarePopCnt)
		fprintf(stdout, "using hardware POPCNT\n");

	if (parent)
	{
		WinParId = GetProcessId(GetCurrentProcess());
		HANDLE JOB = CreateJobObject(NULL, NULL);
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
		jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(JOB, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
		AssignProcessToJobObject(JOB, GetCurrentProcess());

		GetSystemInfo(&sysinfo);
		CPUs = sysinfo.dwNumberOfProcessors;
		PrN = Min(CPUs, MaxPrN);
	}

	init();

	StreamHandle = GetStdHandle(STD_INPUT_HANDLE);
	Console = GetConsoleMode(StreamHandle, &p);

	if (Console)
	{
		SetConsoleMode(StreamHandle, p &(~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT)));
		FlushConsoleInputBuffer(StreamHandle);
	}

	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	fflush(NULL);
}

void send_position(GPos* Pos)
{
	Pos->position->key = Current->key;
	Pos->position->pawn_key = Current->pawn_key;
	Pos->position->move = Current->move;
	Pos->position->capture = Current->capture;
	Pos->position->turn = Current->turn;
	Pos->position->castle_flags = Current->castle_flags;
	Pos->position->ply = Current->ply;
	Pos->position->ep_square = Current->ep_square;
	Pos->position->piece = Current->piece;
	Pos->position->pst = Current->pst;
	Pos->position->material = Current->material;

	for (int i = 0; i < 64; i++)
		Pos->position->square[i] = Board->square[i];
	Pos->date = date;
	Pos->sp = sp;

	for (int i = 0; i <= Current->ply; i++)
		Pos->stack[i] = Stack[sp - i];

	for (int i = 0; i < Min(16, 126 - (int)(Current - Data)); i++)
	{
		Pos->killer[i][0] = (Current + i + 1)->killer[1];
		Pos->killer[i][1] = (Current + i + 1)->killer[2];
	}

	for (int i = Min(16, 126 - (int)(Current - Data)); i < 16; i++)
		Pos->killer[i][0] = Pos->killer[i][1] = 0;
}

void retrieve_board(GPos* Pos)
{
	for (int i = 0; i < 16; i++)
		Board->bb[i] = 0;

	for (int i = 0; i < 64; i++)
	{
		int piece = Pos->position->square[i];
		Board->square[i] = piece;

		if (piece)
		{
			Board->bb[piece & 1] |= Bit(i);
			Board->bb[piece] |= Bit(i);
		}
	}
}

void retrieve_position(GPos* Pos, int copy_stack)
{
	Current->key = Pos->position->key;
	Current->pawn_key = Pos->position->pawn_key;
	Current->move = Pos->position->move;
	Current->capture = Pos->position->capture;
	Current->turn = Pos->position->turn;
	Current->castle_flags = Pos->position->castle_flags;
	Current->ply = Pos->position->ply;
	Current->ep_square = Pos->position->ep_square;
	Current->piece = Pos->position->piece;
	Current->pst = Pos->position->pst;
	Current->material = Pos->position->material;
	retrieve_board(Pos);
	Current->piece_nb = popcnt(PieceAll);
	date = Pos->date;

	if (copy_stack)
	{
		sp = Current->ply;

		for (int i = 0; i <= sp; i++)
			Stack[sp - i] = Pos->stack[i];
	}
	else
		sp = Pos->sp;

	for (int i = 0; i < 16; i++)
	{
		(Current + i + 1)->killer[1] = Pos->killer[i][0];
		(Current + i + 1)->killer[2] = Pos->killer[i][1];
	}
}

void halt_all(GSP* Sp, int locked)
{
	GMove* M;

	if (!locked)
		LOCK(Sp->lock);

	if (Sp->active)
	{
		for (int i = 0; i < Sp->move_number; i++)
		{
			M = &Sp->move[i];

			if ((M->flags & FlagClaimed) && !(M->flags & FlagFinished) && M->id != Id)
				SET_BIT_64(Smpi->stop, M->id);
		}
		Sp->active = Sp->claimed = 0;
		ZERO_BIT_64(Smpi->active_sp, (int)(Sp - Smpi->sp));
	}

	if (!locked)
		UNLOCK(Sp->lock);
}

void halt_all(int from, int to)
{
	for (uint64 u = Smpi->active_sp; u; Cut(u))
	{
		GSP* Sp = &Smpi->sp[lsb(u)];
		LOCK(Sp->lock);

		if (Sp->height >= from && Sp->height <= to)
			halt_all(Sp, 1);
		UNLOCK(Sp->lock);
	}
}

void init_sp(GSP* Sp, int alpha, int beta, int depth, int pv, int singular, int height)
{
	Sp->claimed = 1;
	Sp->active = Sp->finished = 0;
	Sp->best_move = 0;
	Sp->alpha = alpha;
	Sp->beta = beta;
	Sp->depth = depth;
	Sp->split = 0;
	Sp->singular = singular;
	Sp->height = height;
	Sp->move_number = 0;
	Sp->pv = pv;
}

HANDLE CreateChildProcess(int child_id)
{
	char name[1024];
	TCHAR szCmdline[1024] = { 0 };
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	ZeroMemory(szCmdline, 1024 * sizeof(TCHAR));
	ZeroMemory(name, 1024);

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	GetModuleFileName(NULL, name, 1024);
	sprintf(szCmdline, " child %d %d", WinParId, child_id);

	bSuccess = CreateProcess(TEXT(name), TEXT(szCmdline), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo,
		&piProcInfo);

	if (bSuccess)
	{
		CloseHandle(piProcInfo.hThread);
		return piProcInfo.hProcess;
	}
	else
	{
		fprintf(stdout, "Error %d\n", GetLastError());
		return NULL;
	}
}

void init_shared()
{
	char name[256] = { 0 };
	sint64 size = SharedPVHashOffset + pv_hash_size * sizeof(GPVEntry);
	sprintf(name, "Seagull_Shared_%d", WinParId);

	if (parent && SHARED != NULL)
	{
		UnmapViewOfFile(Smpi);
		CloseHandle(SHARED);
	}

	if (parent)
		SHARED = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, DWORD(size), name);
	else
		SHARED = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, name);
	Smpi = (GSMPI *)MapViewOfFile(SHARED, FILE_MAP_ALL_ACCESS, 0, 0, size);

	if (parent)
		memset(Smpi, 0, size);
	Material = (GMaterial *)(((char *)Smpi) + SharedMaterialOffset);
	MagicAttacks = (uint64 *)(((char *)Smpi) + SharedMagicOffset);
	PVHash = (GPVEntry *)(((char *)Smpi) + SharedPVHashOffset);

	if (parent)
		memset(PVHash, 0, pv_hash_size * sizeof(GPVEntry));
}


void check_state()
{
	int n = 0, nc = 0, score = 0, best = 0, pv = 0, alpha = 0, beta = 0;
	int new_depth = 0, r_depth = 0, ext = 0, move = 0, value = 0;
	GSP* Sp = NULL;
	GSP* Spc = NULL;
	GMove* M = NULL;

	if (parent)
	{
		for (uint64 u = TEST_RESET(Smpi->fail_high); u; Cut(u))
		{
			Sp = &Smpi->sp[lsb(u)];
			LOCK(Sp->lock);

			if (Sp->active && Sp->finished)
			{
				UNLOCK(Sp->lock);
				longjmp(Sp->jump, 1);
			}
			UNLOCK(Sp->lock);
		}
		return;
	}

start:
	if (TEST_RESET_BIT(Smpi->stop, Id))
		longjmp(CheckJump, 1);

	if (Smpi->searching & Bit(Id))
		return;

	if (!(Smpi->searching & 1))
	{
		Sleep(1);
		return;
	}

	while ((Smpi->searching & 1) && !Smpi->active_sp)
		_mm_pause();

	while ((Smpi->searching & 1) && !(Smpi->searching & Bit(Id - 1)))
		_mm_pause();

	Sp = NULL;
	best = -0x7FFFFFFF;

	for (uint64 u = Smpi->active_sp; u; Cut(u))
	{
		Spc = &Smpi->sp[lsb(u)];

		if (!Spc->active || Spc->finished || Spc->lock)
			continue;

		for (nc = Spc->current + 1; nc < Spc->move_number; nc++)
			if (!(Spc->move[nc].flags & FlagClaimed))
				break;

		if (nc < Spc->move_number)
			score = 1024 * 1024 + 512 * 1024 * (Spc->depth >= 20) + 128 * 1024 * (!(Spc->split))
			+ ((Spc->depth + 2 * Spc->singular) * 1024) - (((16 * 1024) * (nc - Spc->current)) / nc);
		else
			continue;

		if (score > best)
		{
			best = score;
			Sp = Spc;
			n = nc;
		}
	}

	if (Sp == NULL)
		goto start;

	if (!Sp->active || Sp->finished || (Sp->move[n].flags & FlagClaimed) || n <= Sp->current || n >= Sp->move_number)
		goto start;

	if (Sp->lock)
		goto start;

	LOCK(Sp->lock);

	if (!Sp->active || Sp->finished || (Sp->move[n].flags & FlagClaimed) || n <= Sp->current || n >= Sp->move_number)
	{
		UNLOCK(Sp->lock);
		goto start;
	}

	M = &Sp->move[n];
	M->flags |= FlagClaimed;
	M->id = Id;
	Sp->split |= Bit(Id);
	pv = Sp->pv;
	alpha = Sp->alpha;
	beta = Sp->beta;
	new_depth = M->reduced_depth;
	r_depth = M->research_depth;
	ext = M->ext;
	move = M->move;

	Current = Data;
	retrieve_position(Sp->position, 1);
	evaluate();
	SET_BIT_64(Smpi->searching, Id);
	UNLOCK(Sp->lock);

	if (setjmp(CheckJump))
	{
		ZERO_BIT_64(Smpi->searching, Id);
		return;
	}

	if (Current->turn == White)
	{
		do_move <0>(move);

		if (pv)
		{
			value = -search<1, 0>(-alpha, new_depth, FlagNeatSearch | ExtFlag(ext));

			if (value > alpha)
				value = -pv_search<1, 0>(-beta, -alpha, r_depth, ExtFlag(ext));
		}
		else
		{
			value = -search<1, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));

			if (value >= beta && new_depth < r_depth)
				value = -search<1, 0>(1 - beta, r_depth, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		}
		undo_move <0>(move);
	}
	else
	{
		do_move <1>(move);

		if (pv)
		{
			value = -search<0, 0>(-alpha, new_depth, FlagNeatSearch | ExtFlag(ext));

			if (value > alpha)
				value = -pv_search<0, 0>(-beta, -alpha, r_depth, ExtFlag(ext));
		}
		else
		{
			value = -search<0, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));

			if (value >= beta && new_depth < r_depth)
				value = -search<0, 0>(1 - beta, r_depth, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		}
		undo_move <1>(move);
	}

	LOCK(Sp->lock);
	ZERO_BIT_64(Smpi->searching, Id);

	if (TEST_RESET_BIT(Smpi->stop, Id))
	{
		UNLOCK(Sp->lock);
		return;
	}
	M->flags |= FlagFinished;

	if (value > Sp->alpha)
	{
		Sp->alpha = Min(value, beta);
		Sp->best_move = move;

		if (value >= beta)
		{
			Sp->finished = 1;
			SET_BIT_64(Smpi->fail_high, (int)(Sp - Smpi->sp));
		}
	}
	UNLOCK(Sp->lock);
}