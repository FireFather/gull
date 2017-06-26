#ifndef DEF_H_INCLUDED
#define DEF_H_INCLUDED

#pragma warning(disable : 4996)// 'sscanf': This function or variable may be unsafe

//#define W32_BUILD

#ifdef W32_BUILD
#define NTDDI_VERSION 0x05010200
#define _WIN32_WINNT 0x0501
#endif

#ifndef W32_BUILD
#define HNI
#undef HNI
#endif

#define ENGINE "Seagull"
#define VERSION "1.0"
#define AUTHOR "ThinkingALot"

#ifndef W32_BUILD
#define PLATFORM "x64"
#else
#define PLATFORM "w32"
#endif

#ifndef W32_BUILD
#define MaxPrN 64
#else
#define MaxPrN 32
#endif

typedef unsigned char uint8;
typedef char sint8;
typedef unsigned short uint16;
typedef short sint16;
typedef unsigned int uint32;
typedef int sint32;
typedef unsigned long long uint64;
typedef long long sint64;

#define SplitDepth 10
#define SplitDepthPV 4
#define MaxSplitPoints 64

#define CanCastle_OO 1
#define CanCastle_oo 2
#define CanCastle_OOO 4
#define CanCastle_ooo 8

#define MatWQ 1
#define MatBQ 3
#define MatWR (3 * 3)
#define MatBR (3 * 3 * 3)
#define MatWL (3 * 3 * 3 * 3)
#define MatBL (3 * 3 * 3 * 3 * 2)
#define MatWD (3 * 3 * 3 * 3 * 2 * 2)
#define MatBD (3 * 3 * 3 * 3 * 2 * 2 * 2)
#define MatWN (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2)
#define MatBN (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2 * 3)
#define MatWP (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2 * 3 * 3)
#define MatBP (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2 * 3 * 3 * 9)

#define White 0
#define Black 1
#define WhitePawn 2
#define BlackPawn 3
#define WhiteKnight 4
#define BlackKnight 5
#define WhiteLight 6
#define BlackLight 7
#define WhiteDark 8
#define BlackDark 9
#define WhiteRook 10
#define BlackRook 11
#define WhiteQueen 12
#define BlackQueen 13
#define WhiteKing 14
#define BlackKing 15

#define FlagCastling 0x1000
#define FlagEP 0x2000
#define FlagPKnight 0x4000
#define FlagPLight 0x6000
#define FlagPDark 0x8000
#define FlagPRook 0xA000
#define FlagPQueen 0xC000

#define KpkValue 300

#define EvalValue 26000
#define WdlValue 28000
#define DtzValue 29000
#define MateValue 32760
#define MateScore (MateValue - 120)

#define TimeSingTwoMargin 20
#define TimeSingOneMargin 30
#define TimeNoPVSCOMargin 60
#define TimeNoChangeMargin 70
#define TimeRatio 120
#define PonderRatio 120
#define MovesTg 30
#define InfoLag 5000
#define InfoDelay 1000

#define MagicSize 107648

#define PValue 90
#define NValue 325
#define BValue 325
#define RValue 510
#define QValue 975
#define BPValue 50
#define KValue 30000

#define Phase0 90
#define Phase1 325
#define Phase2 325
#define Phase3 510
#define Phase4 975

#define MatRB Compose (13, -13)
#define MatRN Compose (10, -9)
#define MatQRR Compose (8, 12)
#define MatQRB Compose (4, 6)
#define MatQRN Compose (5, 9)
#define MatQ3 Compose (-3, -8)
#define MatBBR Compose (-4, 7)
#define MatBNR Compose (2, 0)
#define MatNNR Compose (0, -6)
#define MatM Compose (1, 3)

#define StormBlockedMulQuad 126
#define StormShelterAttMulQuad 328
#define StormConnectedMulQuad 463
#define StormOpenMulQuad 215
#define StormFreeMulQuad 89

#define StormBlockedMulLinear 83
#define StormShelterAttMulLinear 156
#define StormConnectedMulLinear 438
#define StormOpenMulLinear 321
#define StormFreeMulLinear 12
#define StormOfValue 22

#define PasserOpRookBlock Compose (0, 13)
#define PasserTarget Compose (-5, -1)

#define UpBlocked Compose (4, 5)
#define ChainRoot Compose (9, -1)

#define IsolatedOpen Compose (6, 6)
#define IsolatedClosed Compose (8, 2)
#define IsolatedBlocked Compose (-8, 0)
#define IsolatedDoubledOpen Compose (-1, 10)
#define IsolatedDoubledClosed Compose (7, 9)

#define BackwardOpen Compose (17, 10)
#define BackwardClosed Compose (4, 1)

#define DoubledOpen Compose (3, 0)
#define DoubledClosed Compose (1, 0)

#define RookHof Compose (8, 0)
#define RookHofWeakPAtt Compose (2, 0)
#define RookOf Compose (11, 8)
#define RookOfOpen Compose (-1, 2)
#define RookOfMinorFixed Compose (-1, -1)
#define RookOfMinorHanging Compose (14, -1)
#define RookOfKingAtt Compose (5, -5)
#define Rook7th Compose (-5, 0)
#define Rook7thK8th Compose (-6, 8)
#define Rook7thDoubled Compose (-7, 31)

#define TacticalMajorPawn Compose (-1, 5)
#define TacticalMinorPawn Compose (0, 5)
#define TacticalMajorMinor Compose (11, 29)
#define TacticalMinorMinor Compose (23, 32)
#define TacticalThreat Compose (19, 11)
#define TacticalDoubleThreat Compose (41, 12)

#define KingDefKnight Compose (2, 0)
#define KingDefBishop Compose (0, 1)
#define KingDefQueen Compose (4, 0)

#define PawnChainLinear Compose (11, 9)
#define PawnChain Compose (9, 4)
#define PawnBlocked Compose (0, 9)
#define PawnFileSpan Compose (1, 1)

#define BishopPawnBlock Compose (0, 3)

#define KnightOutpost Compose (11, 7)
#define KnightOutpostProtected Compose (23, 0)
#define KnightOutpostPawnAtt Compose (13, 6)
#define KnightOutpostBishopAtt Compose (1, 5)
#define KnightOutpostKingAtt Compose (26, 6)

#define WeakPin Compose (21, 39)
#define StrongPin Compose(6, 80)
#define ThreatPin Compose (45, 29)
#define SelfPawnPin Compose (8, 9)
#define SelfPiecePin Compose (48, 27)

#define QKingRay Compose (4, 8)
#define RKingRay Compose (-4, 11)
#define BKingRay Compose (11, -3)

#define cm_pawn 200
#define cm_minor 500
#define cm_rook 700
#define cm_queen 1400

#define pvs_dmin 6
#define pvs_hds 8 
#define pvs_nrnhmds 2
#define pvs_nrhmds 4
#define pvs_nds 8
#define pvs_abs 8
#define pvs_hmds 8
#define pvs_iidlmax 5
#define pvs_pext 2
#define pvs_ce 2
#define pvs_hmdmin 12
#define pvs_spemax 1
#define pvs_smin 2
#define pvs_spemin 2
#define pvs_evsmin 2
#define pvs_evss 1
#define pvs_ndse 2
#define pvs_rdm 6
#define pvs_cm 3
#define pvs_rs 1
#define pvs_rs2 1
#define pvs_rmin 2
#define pvs_ndmax 3
#define pvs_Mrds 2
#define pvs_npvs 1

#define qe_css 10
#define qe_dma 10

#define qs_secsm 3
#define qs_hmmd 8
#define qs_csm 50
#define qs_smv 92 // 50
#define qs_csa 30
#define qs_cmcss 6
#define qs_csgtam 50

#define s_vs 28 // 90
#define s_vmds 5
#define s_vmaxd 13
#define s_va 50
#define s_qsdmax 3
#define s_dmin 20
#define s_maxd 10
#define s_rsmind 12
#define s_nds 8
#define s_minnd 4
#define s_csa 3
#define s_hdds 10
#define s_nmdmax 12
#define s_hds 12
#define s_vgbdmax 12
#define s_cedmax 16
#define s_hmemd 16
#define s_hmeds 12
#define s_sesmax 2
#define s_sepemin 2
#define s_dmeme 16
#define s_meb 2
#define s_hmnds 2
#define s_rsmds 12
#define s_csm 150 // 70
#define s_mmds 7
#define s_vemdmax 19
#define s_vcsrm 250 // 200
#define s_cgfmd 5
#define s_csedmax 16
#define s_ndse 2
#define s_rmd 6
#define s_ndrmin 3
#define s_smv 50
#define s_smra 2
#define s_rmax 2
#define s_rmc 3
#define s_cgtr 2
#define s_ndmax 3
#define s_ncva 10
#define s_ncmd 3
#define s_nccmin 7
#define s_msbcmult 25
#define s_ncdmax 19
#define s_cdmin 9
#define s_smv2 36 // 50
#define s_csrcdmin 9
#define s_smv3 50
#define s_csbcdmax 5
#define s_Mrds 2
#define s_mbsdmax 10

#define se_ldmds 8
#define se_md 20
#define se_cedmax 16
#define se_css 10
#define se_csdmax 3
#define se_dmin 16
#define se_ndmds 12
#define se_vcsdmm 10
#define se_vcsdmmd 3
#define se_dmd 6
#define se_cmc 3

#define r_lds 8
#define r_edms 12
#define r_delta 8
#define r_dmin 626 // 512
#define r_pvs 20 // 50

#endif