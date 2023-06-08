#include <windows.h>
#include <iostream>
#include "def.h"
#include "macro.h"
#include "struct.h"
#include "seagull.h"

uint64 hash_mask = (initial_hash_size - 4);
GEntry* Hash;
static HANDLE HASH = NULL;
sint64 hash_size = initial_hash_size;

void init_hash()
{
	char name[256] = { 0 };
	sint64 size = (hash_size * sizeof(GEntry));
	sprintf(name, "Seagull_Hash_%d", WinParId);
	int hash_initialized = 0;

	if (parent && HASH != NULL)
	{
		hash_initialized = 1;
		UnmapViewOfFile(Hash);
		CloseHandle(HASH);
	}

	if (parent)
	{
		if (!LargePages)
			goto no_lp;

		typedef int(*GETLARGEPAGEMINIMUM)(void);
		GETLARGEPAGEMINIMUM pGetLargePageMinimum;
		HINSTANCE hDll = LoadLibrary(TEXT("kernel32.dll"));

		if (hDll == NULL)
			goto no_lp;
		pGetLargePageMinimum = (GETLARGEPAGEMINIMUM)GetProcAddress(hDll, "GetLargePageMinimum");

		if (pGetLargePageMinimum == NULL)
			goto no_lp;
		int min_page_size = (*pGetLargePageMinimum)();

		if (size < min_page_size)
			size = min_page_size;

		if (!hash_initialized)
		{
			TOKEN_PRIVILEGES tp;
			HANDLE hToken;
			OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
			LookupPrivilegeValue(NULL, "SeLockMemoryPrivilege", &tp.Privileges[0].Luid);
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
		}
		HASH = NULL;
		HASH = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT | SEC_LARGE_PAGES, size >> 32,
			size & 0xFFFFFFFF, name);

		if (HASH != NULL)
		{
			fprintf(stdout, "using large page hash\n");
			goto hash_allocated;
		}
	no_lp:
		HASH = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, size >> 32, size & 0xFFFFFFFF, name);
	}
	else
		HASH = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, name);
hash_allocated:
	Hash = (GEntry *)MapViewOfFile(HASH, FILE_MAP_ALL_ACCESS, 0, 0, size);

	if (parent)
		memset(Hash, 0, size);
	hash_mask = hash_size - 4;
}

__forceinline GEntry* probe_hash()
{
	for (GEntry*Entry = Hash + (High32(Current->key) & hash_mask);
		Entry < (Hash + (High32(Current->key) & hash_mask)) + 4; Entry++)
		if (Low32(Current->key) == Entry->key)
		{
			Entry->date = date;
			return Entry;
		}
	return NULL;
}

__forceinline GPVEntry* probe_pv_hash()
{
	for (GPVEntry*PVEntry = PVHash + (High32(Current->key) & pv_hash_mask);
		PVEntry < PVHash + (High32(Current->key) & pv_hash_mask) + pv_cluster_size; PVEntry++)
		if (Low32(Current->key) == PVEntry->key)
		{
			PVEntry->date = date;
			return PVEntry;
		}
	return NULL;
}

void hash_high(int value, int depth)
{
	int i = 0, score = 0;
	int min_score = 0x70000000;
	GEntry* best = NULL;
	GEntry* Entry = NULL;

	for (i = 0, best = Entry = Hash + (High32(Current->key) & hash_mask); i < 4; i++, Entry++)
	{
		if (Entry->key == Low32(Current->key))
		{
			Entry->date = date;

			if (depth > Entry->high_depth || (depth == Entry->high_depth && value < Entry->high))
			{
				if (Entry->low <= value)
				{
					Entry->high_depth = depth;
					Entry->high = value;
				}
				else if (Entry->low_depth < depth)
				{
					Entry->high_depth = depth;
					Entry->high = value;
					Entry->low = value;
				}
			}
			return;
		}
		else
			score = (Convert(Entry->date, int) << 3) + Convert(Max(Entry->high_depth, Entry->low_depth), int);

		if (score < min_score)
		{
			min_score = score;
			best = Entry;
		}
	}
	best->date = date;
	best->key = Low32(Current->key);
	best->high = value;
	best->high_depth = depth;
	best->low = 0;
	best->low_depth = 0;
	best->move = 0;
	best->flags = 0;
	return;
}

void hash_low(int move, int value, int depth)
{
	int i = 0, score = 0;
	int min_score = 0x70000000;
	GEntry* best = NULL;
	GEntry* Entry = NULL;

	move &= 0xFFFF;

	for (i = 0, best = Entry = Hash + (High32(Current->key) & hash_mask); i < 4; i++, Entry++)
	{
		if (Entry->key == Low32(Current->key))
		{
			Entry->date = date;

			if (depth > Entry->low_depth || (depth == Entry->low_depth && value > Entry->low))
			{
				if (move)
					Entry->move = move;

				if (Entry->high >= value)
				{
					Entry->low_depth = depth;
					Entry->low = value;
				}
				else if (Entry->high_depth < depth)
				{
					Entry->low_depth = depth;
					Entry->low = value;
					Entry->high = value;
				}
			}

			else if (F(Entry->move))
				Entry->move = move;
			return;
		}
		else
			score = (Convert(Entry->date, int) << 3) + Convert(Max(Entry->high_depth, Entry->low_depth), int);

		if (score < min_score)
		{
			min_score = score;
			best = Entry;
		}
	}
	best->date = date;
	best->key = Low32(Current->key);
	best->high = 0;
	best->high_depth = 0;
	best->low = value;
	best->low_depth = depth;
	best->move = move;
	best->flags = 0;
	return;
}
void hash_exact(int move, int value, int depth, int exclusion, int ex_depth, int knodes)
{
	int i = 0, score = 0;
	int min_score = 0x70000000;
	GPVEntry* best = NULL;
	GPVEntry* PVEntry = NULL;

	for (i = 0, best = PVEntry = PVHash + (High32(Current->key) & pv_hash_mask); i < pv_cluster_size; i++, PVEntry++)
	{
		if (PVEntry->key == Low32(Current->key))
		{
			PVEntry->date = date;
			PVEntry->knodes += knodes;

			if (PVEntry->depth <= depth)
			{
				PVEntry->value = value;
				PVEntry->depth = depth;
				PVEntry->move = move;
				PVEntry->ply = Current->ply;

				if (ex_depth)
				{
					PVEntry->exclusion = exclusion;
					PVEntry->ex_depth = ex_depth;
				}
			}
			return;
		}
		score = (Convert(PVEntry->date, int) << 3) + Convert(PVEntry->depth, int);

		if (score < min_score)
		{
			min_score = score;
			best = PVEntry;
		}
	}
	best->key = Low32(Current->key);
	best->date = date;
	best->value = value;
	best->depth = depth;
	best->move = move;
	best->exclusion = exclusion;
	best->ex_depth = ex_depth;
	best->knodes = knodes;
	best->ply = Current->ply;
}

