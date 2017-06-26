#ifndef MACRO_H_INCLUDED
#define MACRO_H_INCLUDED

#define Convert(x,type) ((type)(x))

#define Abs(x) ((x) > 0 ? (x) : (-(x)))
#define Sgn(x) ((x) == 0 ? 0 : ((x) > 0 ? 1 : (-1)))
#define Min(x, y) ((x) < (y) ? (x) : (y))
#define Max(x, y) ((x) > (y) ? (x) : (y))
#define Sqr(x) ((x) * (x))
#define T(x) ((x) != 0)
#define F(x) ((x) == 0)
#define Even(x) F((x) & 1)
#define Odd(x) T((x) & 1)
#define Combine(x, y) ((x) | ((y) << 16))
#define Compose(x, y) ((x) + ((y) << 16))
#define Compose16(x, y) Compose(((x) >> 4), ((y) >> 4))
#define Compose64(x, y) Compose(((x) >> 6), ((y) >> 6))
#define Compose256(x, y) Compose(((x) >> 8), ((y) >> 8))
#define Opening(x) Convert((x) & 0xFFFF, sint16)
#define Endgame(x) ((((x) >> 15) & 1) + Convert((x) >> 16, sint16))

#define File(x) ((x) & 7)
#define Rank(x) ((x) >> 3)
#define CRank(me, x) ((me) ? (7 - Rank(x)) : Rank(x))
#define NDiag(x) (7 - File(x) + Rank(x))
#define SDiag(x) (File(x) + Rank(x))
#define Dist(x, y) Max(Abs(Rank(x) - Rank(y)), Abs(File(x) - File(y)))
#define VarC(var, me) ((me) ? (var##_b) : (var##_w))
#define PVarC(prefix, var, me) ((me) ? (prefix##.##var##_b) : (prefix##.##var##_w))

#define Bit(x) (Convert(1, uint64) << (x))

#ifndef HNI
#define Cut(x) (x &= (x) - 1)
#else
#define Cut(x) (x = _blsr_u64(x))
#endif

#define Multiple(x) T((x) & ((x) - 1))
#define Single(x) F((x) & ((x) - 1))
#define Add(x, b) (x |= Bit(b))

#define From(move) (((move) >> 6) & 0x3f)
#define To(move) ((move) & 0x3f)
#define SetScore(move, score) ((move) = (((move) & 0xFFFF) | ((score) << 16)))
#define BitFrom(move) Bit(From(move))
#define BitTo(move) Bit(To(move))
#define MakeMove(from, to) ((from) << 6) | (to))
#define MakeMoveF(from, to, flags) ((from) << 6) | (to) | (flags))
#define MakeMoveFS(from, to, flags, score) ((from) << 6) | (to) | (flags) | (score))
#define PieceAtOrigin(move) Square(From(move))
#define Target(move) Square(To(move))

#define Empty Convert(0, uint64)
#define Filled (~Empty)
#define Interior Convert(0x007E7E7E7E7E7E00, uint64)
#define Boundary (~Interior)
#define WhiteArea Convert(0x00000000FFFFFFFF, uint64)
#define BlackArea (~WhiteArea)
#define LightArea Convert(0x55AA55AA55AA55AA, uint64)
#define DarkArea (~LightArea)
#define FileA Convert(0x0101010101010101, uint64)
#define Line0 Convert(0x00000000000000FF, uint64)

#define High32(x) ((x) >> 32)
#define Low32(x) Convert(x, uint32)

#define IsSlider(x) T(0x3FC0 & Bit(x))
#define IsPromotion(move) T((move) & 0xC000)
#define IsCastling(move) T((move) & 0x1000)
#define IsEP(move) (((move) & 0xF000) == 0x2000)
#define Promotion(move, side) ((side) + (((move) & 0xF000) >> 12))

#ifndef HNI
#define BishopAttacks(sq, occ) (*(BOffsetPointer[sq] + (((BMagicMask[sq] & (occ)) * BMagic[sq]) >> BShift[sq])))
#define RookAttacks(sq, occ) (*(ROffsetPointer[sq] + (((RMagicMask[sq] & (occ)) * RMagic[sq]) >> RShift[sq])))
#else
#define BishopAttacks(sq, occ) (*(BOffsetPointer[sq] + _pext_u64(occ, BMagicMask[sq])))
#define RookAttacks(sq, occ) (*(ROffsetPointer[sq] + _pext_u64(occ, RMagicMask[sq])))
#endif
#define QueenAttacks(sq, occ) (BishopAttacks(sq,occ) | RookAttacks(sq,occ))

#define TotalMat (((MatWQ + MatBQ) << 1) + MatWL + MatBL + MatWD + MatBD + \
	((MatWR + MatBR + MatWN + MatBN) << 1) + ((MatWP + MatBP) << 3) + 1)
#define FlagUnusualMaterial (1 << 30)

#define opp (1 ^ (me))

#define IPawn(me) (WhitePawn | (me))
#define IKnight(me) (WhiteKnight | (me))
#define ILight(me) (WhiteLight | (me))
#define IDark(me) (WhiteDark | (me))
#define IRook(me) (WhiteRook | (me))
#define IQueen(me) (WhiteQueen | (me))
#define IKing(me) (WhiteKing | (me))

#define BB(i) Board->bb[i]
#define Pawn(me) (BB(WhitePawn | (me)))
#define Knight(me) (BB(WhiteKnight | (me)))
#define Bishop(me) (BB(WhiteLight | (me)) | BB(WhiteDark | (me)))
#define Rook(me) (BB(WhiteRook | (me)))
#define Queen(me) (BB(WhiteQueen | (me)))
#define King(me) (BB(WhiteKing | (me)))
#define Piece(me) (BB(me))
#define NonPawn(me) (Piece(me) ^ Pawn(me))
#define NonPawnKing(me) (NonPawn(me) ^ King(me))
#define BSlider(me) (Bishop(me) | Queen(me))
#define RSlider(me) (Rook(me) | Queen(me))
#define Major(me) RSlider(me)
#define Minor(me) (Knight(me) | Bishop(me))
#define Slider(me) (BSlider(me) | RSlider(me))
#define PieceAll (Piece(White) | Piece(Black))
#define SliderAll (Slider(White) | Slider(Black))
#define PawnAll (Pawn(White) | Pawn(Black))
#define NonPawnKingAll (NonPawnKing(White) | NonPawnKing(Black))
#define KingPos(me) (lsb(King(me)))

#define ShiftNW(target) (((target) & (~(File[0] | Line[7]))) << 7)
#define ShiftNE(target) (((target) & (~(File[7] | Line[7]))) << 9)
#define ShiftSE(target) (((target) & (~(File[7] | Line[0]))) >> 7)
#define ShiftSW(target) (((target) & (~(File[0] | Line[0]))) >> 9)
#define ShiftW(me, target) ((me) ? ShiftSW(target) : ShiftNW(target))
#define ShiftE(me, target) ((me) ? ShiftSE(target) : ShiftNE(target))
#define ShiftN(target) ((target) << 8)
#define ShiftS(target) ((target) >> 8)
#define Shift(me, target) ((me) ? ShiftS(target) : ShiftN(target))
#define PushW(me) ((me) ? (-9) : (7))
#define PushE(me) ((me) ? (-7) : (9))
#define Push(me) ((me) ? (-8) : (8))
#define Dir(me) ((me) ? (-1) : (1))
#define IsGreater(me, x, y) ((me) ? ((x) < (y)) : ((x) > (y)))

#define Line(me, n) ((me) ? Line[7 - n] : Line[n])
#define Square(sq) Board->square[sq]
#define AddMove(from, to, flags, score) { *list = ((from) << 6) | (to) | (flags) | (score); list++; }
#define AddCapture(from, to, flags) AddMove(from, to, flags, MvvLva[Square(from)][Square(to)])
#define AddCaptureP(piece, from,to ,flags) AddMove(from, to, flags, MvvLva[piece][Square(to)])
#define AddHistoryP(piece, from, to, flags) AddMove(from, to, flags, HistoryP(piece, from, to))
#define AddHistory(from, to) AddMove(from, to, 0, History(from,to))
#define AddDeltaP(piece, from, to, flags) \
	AddMove(from, to, flags, Convert(DeltaScore(piece, from, to) + (sint16)0x4000, int) << 16)
#define AddDelta(from, to) AddMove(from, to, 0, Convert(Delta(from, to) + (sint16)0x4000, int) << 16)
#define AddCDeltaP(piece, from, to, flags) \
	{if (DeltaScore(piece, from, to) >= Current->margin) \
	AddMove(from, to, flags, Convert(DeltaScore(piece, from, to)+(sint16)0x4000, int) << 16)}
#define AddCDelta(from, to) {if (Delta(from, to) >= Current->margin)\
	AddMove(from, to, 0, Convert(Delta(from, to) + (sint16)0x4000, int) << 16)}

#define Check(me) T(Current->att[(me) ^ 1] & King(me))
#define IsIllegal(me,move) ((T(Current->xray[opp] & Bit(From(move))) && F(Bit(To(move)) & FullLine[lsb(King(me))][From(move)])) \
    || (IsEP(move) && T(Line[Rank(From(move))] & King(me)) && T(Line[Rank(From(move))] & Major(opp)) &&                         \
    T(RookAttacks(lsb(King(me)),PieceAll ^ Bit(From(move)) ^ Bit(Current->ep_square - Push(me))) & Major(opp))))
#define IsRepetition(margin,move) \
	((margin) > 0 && Current->ply >= 2 && (Current-1)->move == ((To(move) << 6) | From(move)) && F(Square(To(move))) && F((move) & 0xF000))

#define IncV(var,x) (me ? (var -= (x)) : (var += (x)))
#define DecV(var,x) IncV(var,-(x))

#define PawnCaptureMvvLva(attacker) (MvvLvaAttacker[attacker])
#define MaxPawnCaptureMvvLva (MvvLvaAttacker[15])
#define KnightCaptureMvvLva(attacker) (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[attacker])
#define MaxKnightCaptureMvvLva (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[15])
#define BishopCaptureMvvLva(attacker) (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[attacker] + 1)
#define MaxBishopCaptureMvvLva (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[15] + 1)
#define RookCaptureMvvLva(attacker) (MaxBishopCaptureMvvLva + MvvLvaAttacker[attacker])
#define MaxRookCaptureMvvLva (MaxBishopCaptureMvvLva + MvvLvaAttacker[15])
#define QueenCaptureMvvLva(attacker) (MaxRookCaptureMvvLva + MvvLvaAttacker[attacker])

#define MvvLvaPromotion (MvvLva[WhiteQueen][BlackQueen])
#define MvvLvaPromotionKnight (MvvLva[WhiteKnight][BlackKnight])
#define MvvLvaPromotionCap(capture) (MvvLva[((capture) < WhiteRook) ? WhiteRook : ((capture) >= WhiteQueen ? WhiteKing : WhiteKnight)][BlackQueen])
#define MvvLvaPromotionKnightCap(capture) (MvvLva[WhiteKing][capture])
#define MvvLvaXray (MvvLva[WhiteQueen][WhitePawn])
#define MvvLvaXrayCap(capture) (MvvLva[WhiteKing][capture])
#define RefOneScore ((0xFF << 16) | (3 << 24))
#define RefTwoScore ((0xFF << 16) | (2 << 24))
#define KillerOneScore ((0xFF << 16) | (1 << 24))
#define KillerTwoScore (0xFF << 16)

#define halt_check if ((Current - Data) >= 126) {evaluate(); return Current->score;} \
    if (Current->ply >= 100) return 0;                                               \
    for (i = 4; i <= Current->ply; i += 2) if (Stack[sp-i] == Current->key) return 0

#define ExtFlag(ext) ((ext) << 16)
#define Ext(flags) (((flags) >> 16) & 0xF)
#define FlagHashCheck (1 << 20)
#define FlagHaltCheck (1 << 21)
#define FlagCallEvaluation (1 << 22)
#define FlagDisableNull (1 << 23)
#define FlagNeatSearch (FlagHashCheck | FlagHaltCheck | FlagCallEvaluation)
#define FlagNoKillerUpdate (1 << 24)
#define FlagReturnBestMove (1 << 25)

#define MSBZ(x) ((x) ? msb(x) : 63)
#define LSBZ(x) ((x) ? lsb(x) : 0)
#define NB(me, x) ((me) ? msb(x) : lsb(x))
#define NBZ(me, x) ((me) ? MSBZ(x) : LSBZ(x))

#define StageNone ((1 << s_none) | (1 << e_none) | (1 << r_none))
#define initial_hash_size (1024 * 1024)

#define FlagSort (1 << 0)
#define FlagNoBcSort (1 << 1)
#define pawn_hash_size (1024 * 1024)
#define pawn_hash_mask (pawn_hash_size - 1)
#define pv_hash_size (1024 * 1024)
#define pv_cluster_size 4
#define pv_hash_mask (pv_hash_size - pv_cluster_size)
#define prefetch(a, mode) _mm_prefetch(a,mode)

#define FlagSingleBishop_w (1 << 0)
#define FlagSingleBishop_b (1 << 1)
#define FlagCallEvalEndgame_w (1 << 2)
#define FlagCallEvalEndgame_b (1 << 3)
#define Pst(piece,sq) Pst[((piece) << 6) | (sq)]

#define HistoryScore(piece, from, to) History[((piece) << 6) | (to)]
#define HistoryP(piece, from, to) ((Convert(HistoryScore(piece,from,to) & 0xFF00,int)/Convert(HistoryScore(piece,from,to) & 0x00FF,int)) << 16)
#define History(from, to) HistoryP(Square(from), from, to)
#define HistoryM(move) HistoryScore(Square(From(move)), From(move), To(move))
#define HistoryInc(depth) Min(((depth) >> 1) * ((depth) >> 1), 64)
#define HistoryGood(move) if ((HistoryM(move) & 0x00FF) >= 256 - HistoryInc(depth))                     \
    HistoryM(move) = ((HistoryM(move) & 0xFEFE) >> 1) + ((HistoryInc(depth) << 8) | HistoryInc(depth)); \
    else HistoryM(move) += ((HistoryInc(depth) << 8) | HistoryInc(depth))
#define HistoryBad(move) if ((HistoryM(move) & 0x00FF) >= 256 - HistoryInc(depth))                      \
    HistoryM(move) = ((HistoryM(move) & 0xFEFE) >> 1) + HistoryInc(depth); else HistoryM(move) += HistoryInc(depth)

#define DeltaScore(piece, from, to) Delta[((piece) << 12) | ((from) << 6) | (to)]
#define Delta(from, to) DeltaScore(Square(from), from, to)
#define DeltaM(move) Delta(From(move), To(move))
#define UpdateDelta if (F(Current->capture) && T(Current->move) && F(Current->move & 0xE000) && Current > Data) {     \
    if (DeltaScore(Current->piece, From(Current->move), To(Current->move)) <= -Current->score - ((Current - 1)->score)) \
    DeltaScore(Current->piece, From(Current->move), To(Current->move)) = -Current->score - ((Current - 1)->score);      \
    else DeltaScore(Current->piece, From(Current->move), To(Current->move))--; }
#define DeltaMarginP(piece, from, to) (DeltaScore(piece, from, to) >= Current->margin)
#define DeltaMargin(from, to) (Delta(from, to) >= Current->margin)
#define RefPointer(piece, from, to) Ref[((piece) << 6) | (to)]
#define RefM(move) RefPointer(Square(To(move)), From(move), To(move))
#define UpdateRef(ref_move) if (T(Current->move) && RefM(Current->move).ref[0] != (ref_move)) {            \
    RefM(Current->move).ref[1] = RefM(Current->move).ref[0]; RefM(Current->move).ref[0] = (ref_move); }
#define UpdateCheckRef(ref_move) if (T(Current->move) && RefM(Current->move).check_ref[0] != (ref_move)) { \
    RefM(Current->move).check_ref[1] = RefM(Current->move).check_ref[0]; RefM(Current->move).check_ref[0] = (ref_move); }

#define ArrayIndex(width, row, column) (((row) * (width)) + (column))
#define Av(x, width, row, column) (*((x) + ArrayIndex(width, row, column)))
#define TrAv(x, w, r, c) Av(x, 0, 0, (((r) * (((w) << 1) - (r) + 1)) >> 1) + (c))
#define Sa(x, y) Av(x, 0, 0, y)
#define Ca(x, y) Compose(Av(x, 0, 0, ((y) << 1)), Av(x, 0, 0, ((y) << 1) + 1))

#define MaxPhase ((PValue << 4) + (NValue << 2) + (BValue << 2) + (RValue << 2) +(QValue<< 1))
#define PhaseMin ((RValue << 1) + NValue + BValue)
#define PhaseMax (MaxPhase - NValue - BValue)

#define KingNAttack Compose(1, Av(KingAttackWeight, 0, 0, 0))
#define KingBAttack Compose(1, Av(KingAttackWeight, 0, 0, 1))
#define KingRAttack Compose(1, Av(KingAttackWeight, 0, 0, 2))
#define KingQAttack Compose(1, Av(KingAttackWeight, 0, 0, 3))
#define KingAttack Compose(1, 0)
#define KingAttackSquare Av(KingAttackWeight, 0, 0, 4)
#define KingNoMoves Av(KingAttackWeight, 0, 0, 5)
#define KingShelterQuad Av(KingAttackWeight, 0, 0, 6)

#define FlagClaimed (1 << 1)
#define FlagFinished (1 << 2)

#define ExclSingle(depth) 8
#define ExclDouble(depth) 16
#define ExclSinglePV(depth) 8
#define ExclDoublePV(depth) 16

#define SharedMaterialOffset (sizeof(GSMPI))
#define SharedMagicOffset (SharedMaterialOffset + TotalMat * sizeof(GMaterial))
#define SharedPVHashOffset (SharedMagicOffset + MagicSize * sizeof(uint64))

#ifndef W32_BUILD
#define SET_BIT(var, bit) (InterlockedOr(&(var), 1 << (bit)))
#define SET_BIT_64(var, bit) (InterlockedOr64(&(var), Bit(bit)));
#define ZERO_BIT_64(var, bit) (InterlockedAnd64(&(var), ~Bit(bit)));
#define TEST_RESET_BIT(var ,bit) (InterlockedBitTestAndReset64(&(var), bit))
#define TEST_RESET(var) (InterlockedExchange64(&(var), 0))
#else
#define SET_BIT(var, bit) (_InterlockedOr(&(var),1 << (bit)))
#define SET_BIT_64(var, bit) {if ((bit) < 32) _InterlockedOr((LONG*)&(var),1 << (bit)); else _InterlockedOr(((LONG*)(&(var))) + 1,1 << ((bit) - 32));}
#define ZERO_BIT_64(var, bit) {if ((bit) < 32) _InterlockedAnd((LONG*)&(var),~(1 << (bit))); else _InterlockedAnd(((LONG*)(&(var))) + 1,~(1 << ((bit) - 32)));}
#define TEST_RESET_BIT(var, bit) (InterlockedBitTestAndReset(&(var), bit))
#define TEST_RESET(var) (InterlockedExchange(&(var), 0))
#endif

#define SET(var,value) (InterlockedExchange(&(var), value))
#define LOCK(lock) {while (InterlockedCompareExchange(&(lock), 1, 0)) _mm_pause();}
#define UNLOCK(lock) {SET(lock, 0);}

#endif