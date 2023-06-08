#ifndef SEAGULL_H_INCLUDED
#define SEAGULL_H_INCLUDED

extern int Pst[16 * 64];

extern uint64 RMask[64];
extern uint64 BMask[64];
extern uint64 Between[64][64];
extern uint64* MagicAttacks;
extern uint64 BMagicMask[64];
extern uint64 RMagicMask[64];

extern GEntry* Hash;
extern uint64 hash_mask;
extern __forceinline GEntry* probe_hash();
extern sint64 hash_size;

extern int LargePages;
extern int parent;
extern int WinParId;

extern GMaterial* Material;

extern void gen_kpk();

extern uint64 Forward[2][8];
extern uint64 West[8];
extern uint64 East[8];
extern uint64 PIsolated[8];
extern uint64 HLine[64];
extern uint64 VLine[64];
extern uint64 NDiag[64];
extern uint64 SDiag[64];

extern uint64 QMask[64];
extern uint64 NAtt[64];
extern uint64 SArea[64];
extern uint64 DArea[64];
extern uint64 NArea[64];
extern uint64 BishopForward[2][64];
extern uint64 PAtt[2][64];
extern uint64 PMove[2][64];
extern uint64 PWay[2][64];
extern uint64 PSupport[2][64];
extern uint64 FullLine[64][64];
extern uint64 TurnKey;
extern uint8 PieceFromChar[256];
extern uint64 PieceKey[16][64];
extern uint64 CastleKey[16];
extern uint64 EPKey[8];

extern uint16 date;
extern GPVEntry* PVHash;
extern int best_move;

extern GSearchInfo CurrentSI[1];
extern GSearchInfo BaseSI[1];

extern int best_score;
extern int LastTime;
extern int LastValue;
extern int LastExactValue;
extern int InstCnt;
extern sint64 LastSpeed;
extern int Infinite;
extern int SearchMoves;
extern int TimeLimit1;
extern int TimeLimit2;
extern int Stop;
extern int Searching;
extern int DepthLimit;
extern int LastDepth;
extern void get_board(const char fen[]);
extern uint64 nodes;
extern GSMPI* Smpi;
extern sint16 History[16 * 64];
extern sint16 Delta[16 * 4096];
extern GRef Ref[16 * 64];
__declspec(align(64)) extern GData Data[128];
extern GData* Current;

extern __forceinline int pick_move();
extern int pv_length;
extern int RootList[256];
extern uint16 PV[128];
extern int pvp;
extern char pv_string[1024];
extern char score_string[16];
extern sint64 StartTime;
extern bool BenchMarking;
extern __forceinline GPVEntry* probe_pv_hash();
extern int PVN;
extern int MultiPV[256];
extern void init();
extern uint64* BOffsetPointer[64];
extern uint64* ROffsetPointer[64];

extern int sp;
extern uint64 Stack[2048];
__declspec(align(64)) extern GBoard Board[1];

extern int MoveTime;
extern bool TBs_initialized;

namespace Tablebases
{
	extern int TBLargest;
	void init(const std::string & path);
	int probe_wdl(int*success);
	int probe_dtz(int*success);
}
extern char mstring[65536];
extern int ResetHash;
extern jmp_buf ResetJump;
extern int NewPrN;
extern int Ponder;
extern int PVHashing;
extern int Aspiration;
extern int VerboseUCI;
extern int TBProbeDepth;
extern int TBProbeLimit;
extern int PrN;
extern int CPUs;

template <int me> extern void gen_root_moves();

extern HANDLE ChildPr[MaxPrN];
extern int child;
extern int Id;
extern int HardwarePopCnt;
extern HANDLE StreamHandle;
extern int Console;

extern int Mobility[4][32];
extern int PasserGeneral[8];
extern int PasserBlocked[8];
extern int PasserFree[8];
extern int PasserSupported[8];
extern int PasserProtected[8];
extern int PasserConnected[8];
extern int PasserOutside[8];
extern int PasserCandidate[8];
extern int PasserClear[8];

extern sint16 Shelter[3][8];
extern sint16 StormBlocked[4];
extern sint16 StormShelterAtt[4];
extern sint16 StormConnected[4];
extern sint16 StormOpen[4];
extern sint16 StormFree[4];
extern sint16 PasserAtt[8];
extern sint16 PasserDef[8];
extern sint16 PasserAttLog[8];
extern sint16 PasserDefLog[8];

extern uint64 Kpk[2][64][64];
extern int MvvLva[16][16];
extern uint8 LogDist[16];
extern int PrevMove;
extern sint64 InfoTime;
extern char info_string[1024];
extern jmp_buf Jump;
extern int SMPointer;
extern uint16 SMoves[256];

#endif