#include <windows.h>
#include <iostream>
#include <time.h> 
#include "def.h"
#include "macro.h"
#include "struct.h"
#include "function.h"
#include "seagull.h"

static uint64 TotalTime = 0;
static uint64 TotalNodes = 0;
static FILE *BenchFile;
static char FileName[256];

static char BenchPositions[32][128] =
{
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
	"r2q1rk1/1bppbppp/p4n2/n2Np3/Pp2P3/1B1P1N2/1PP2PPP/R1BQ1RK1 w - -",
	"2rq1rk1/p3bppp/bpn1pn2/2pp4/3P4/1P2PNP1/PBPN1PBP/R2QR1K1 w - -",
	"r1bq1rk1/3nbppp/p1p1pn2/1p4B1/3P4/2NBPN2/PP3PPP/2RQK2R w K -",
	"r1b1k2r/pp1nqp1p/2p3p1/3p3n/3P4/2NBP3/PPQ2PPP/2KR2NR w kq -",
	"r2q1rk1/pp2ppbp/2n1bnp1/3p4/4PPP1/1NN1B3/PPP1B2P/R2QK2R w KQ -",
	"rn3rk1/pbppqpp1/1p2p2p/8/2PP4/2Q1PN2/PP3PPP/R3KB1R w KQ -",
	"rnb2rk1/p4ppp/1p2pn2/q1p5/2BP4/P1P1PN2/1B3PPP/R2QK2R w KQ -",
	"r2q1rk1/3nbppp/bpp1pn2/p1Pp4/1P1P1B2/P1N1PN1P/5PP1/R2QKB1R w KQ -",
	"r3kbnr/1bqp1ppp/p3p3/1p2P3/5P2/2N2B2/PPP3PP/R1BQK2R w KQkq -",
	"r1bq1rk1/pp1n1ppp/4p3/2bpP3/3n1P2/2N1B3/PPPQ2PP/2KR1B1R w - -",
	"2q1r1k1/1ppb4/r2p1Pp1/p4n1p/2P1n3/5NPP/PP3Q1K/2BRRB2 w - -",
	"4k3/p1P3p1/2q1np1p/3N4/8/1Q3PP1/6KP/8 w - -",
	"6k1/p1qb1p1p/1p3np1/2b2p2/2B5/2P3N1/PP2QPPP/4N1K1 b - -",
	"3rr1k1/pb3pp1/1p1q1b1p/1P2NQ2/3P4/P1NB4/3K1P1P/2R3R1 w - -",
	"3r4/1b2k3/1pq1pp2/p3n1pr/2P5/5PPN/PP1N1QP1/R2R2K1 b - -",
	"1r5r/3b1pk1/3p1np1/p1qPp3/p1N1PbP1/2P2PN1/1PB1Q1K1/R3R3 b - -",
	"rnb2rk1/pp2np1p/2p2q1b/8/2BPPN2/2P2Q2/PP4PP/R1B2RK1 w - -",
	"r3kb1r/pp2pppp/3q4/3Pn3/6b1/2N1BN2/PP3PPP/R2QKB1R w KQkq -",
	"r1b1k3/5p1p/p1p5/3np3/1b2N3/4B3/PPP1BPrP/2KR3R w q -",
	"b7/2q2kp1/p3pbr1/1pPpP2Q/1P1N3P/6P1/P7/5RK1 w - -",
	"r7/5kp1/2p1p2p/1p1n3P/2rP4/2P3R1/PK2RPP1/2B5 b - -",
	"2k2R2/6r1/8/B2pp2p/1p6/3P4/PP2b3/2K5 b - -",
	"2n5/1k6/3pNn2/3ppp2/7p/4P2P/1P4P1/5NK1 w - -",
	"8/1p3pkp/p1r3p1/3P3n/3p1P2/3P4/PP3KP1/R3N3 b - -",
	"8/4p1kp/1n1p2p1/nPp5/b5P1/P5KP/3N1P2/4NB2 w - -",
	"1k2b3/1pp5/4r3/R3N1pp/1P3P2/p5P1/2P4P/1K6 w - -",
	"1r6/5ppk/R6p/P3p3/1Pn5/6P1/2p2P1P/2B4K w - -",
	"2n5/7r/1p1k4/2nP1p2/4P3/P3KP1P/3R4/5B2 w - -",
	"3k4/2p3pp/3p1b2/3P3P/b7/P3BB2/1P3P2/2K5 w - -",
	"3rn3/p4p2/1p3k2/6pp/2PpB3/P2K2P1/1P4PP/4R3 b - -",
	"4n3/p5k1/2P3pp/2P5/P3pp2/2K3P1/5r1P/R4N2 w - -",
};

void Bench()
{
	int i = 0;
	uint64 C = 0, Time = 0;
	TotalTime = 0, TotalNodes = 0;
	char * ptr = NULL;
	init_search(1);
	BenchMarking = true;
	while (i < 32)
	{
		fprintf(stdout, "position %d\n", i);
		sprintf(mstring, "%s\n", BenchPositions[i]);
		ptr = strchr(mstring, '\n');
		if (ptr != NULL)
			*ptr = 0;
		get_board(mstring);
		evaluate();
		if (Current->turn == White)
			gen_root_moves<0>();
		else
			gen_root_moves<1>();
		C = GetClock();
		Searching = 1;
		SET_BIT_64(Smpi->searching, Id);
		get_time_limit(mstring);
		Time = GetClock() - C;
		TotalNodes += Smpi->nodes;
		TotalTime += Time;
		fprintf(stdout, "nodes : %lld\n", Smpi->nodes);
		fprintf(stdout, "time  : %lld ms\n", Time / 1000);
		if (Time != 0)
			fprintf(stdout, "NPS   : %lld kNPS\n\n", ((Smpi->nodes * 1000) / Time));
		init_search(1);
		i++;
	}
	fprintf(stdout, "total nodes : %lld\n", TotalNodes);
	fprintf(stdout, "total time  : %lld ms\n", TotalTime / 1000);
	if (TotalTime != 0)
		fprintf(stdout, "total NPS   : %lld kNPS\n\n", ((TotalNodes * 1000) / TotalTime));
	BenchMarking = false;
	CreateBenchLog();
}

static void CreateBenchLog(void)
{
	char buf[256] = { 0 };
	time_t now;
	struct tm tnow;
	time(&now);
	tnow = *localtime(&now);
	strftime(buf, 32, "%H-%M_%b-%d", &tnow);
	sprintf(FileName, "benchmark_%s.txt", buf);
	fprintf(stdout, "results written to:\n");
	fprintf(stdout, "benchmark_%s.txt\n\n", buf);
	BenchFile = fopen(FileName, "wt");
	fprintf(BenchFile, "// " ENGINE " " VERSION " " PLATFORM "\n");
	fprintf(BenchFile, "// benchmark results\n\n");
	fprintf(BenchFile, "total nodes : %lld\n", TotalNodes);
	fprintf(BenchFile, "total time  : %lld ms\n", TotalTime / 1000);
	if (TotalTime != 0)
		fprintf(BenchFile, "total NPS   : %lld kNPS\n", ((TotalNodes * 1000) / TotalTime));
	fclose(BenchFile);
}

uint64 GetClock()
{
	return (GetTickCount64() * 1000ULL);
}