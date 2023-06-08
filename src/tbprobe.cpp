/*
  Copyright (c) 2013 Ronald de Man
  This file may be redistributed and/or modified without restrictions.

  tbprobe.cpp contains the Stockfish-specific routines of the
  tablebase probing code. It should be relatively easy to adapt
  this code to other chess engines.
*/

// The probing code currently expects a little-endian architecture (e.g. x86).

// Define DECOMP64 when compiling for a 64-bit platform.
// 32-bit is only supported for 5-piece tables, because tables are mmap()ed
// into memory.
#ifdef IS_64BIT
#define DECOMP64
#endif

#include "tbprobe.h"
#include "tbcore.h"
#include "seagull.h"
#include "tbcore.cpp"


#define psq(color, piece_type, sq) PieceKey[((piece_type) << 1) | color][sq]
#define WHITE White
#define BLACK Black
#define KING 6
#define QUEEN 5
#define PAWN 1
#define Bitboard uint64

const int piece_from_pt[7] =
    {
    0, 2, 4, 6, 10, 12, 14
    };

const int pt_from_piece[16] =
    {
    0, 0, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6
    };

__forceinline uint64 bb_color_pt(int color, int pt)
    {
    if (pt == 3)
        return Bishop(color);
    else
        return BB(piece_from_pt[pt] | color);
    }

__forceinline int pop_lsb(uint64* x)
    {
    int sq = lsb(*x);
    * x ^= Bit(sq);
    return sq;
    }

int Tablebases::TBLargest = 0;

void compute_pos_info()
    {
    if (Current->eval_key == Current->key)
        return;
    evaluate();
    return;

    Current->patt[White] = ShiftW(White, Pawn(White)) | ShiftE(White, Pawn(White));
    Current->patt[Black] = ShiftW(Black, Pawn(Black)) | ShiftE(Black, Pawn(Black));
    int king_w = lsb(King(White)), king_b = lsb(King(Black));
    Current->att[White] = Current->patt[White] | SArea[king_w];
    Current->att[Black] = Current->patt[Black] | SArea[king_b];
    Current->xray[White] = Current->xray[Black] = 0;
    uint64 occ = PieceAll;

#define me White

    for (uint64 u = Knight(me); u; Cut(u))
        Current->att[me] |= NAtt[lsb(u)];

    for (uint64 u = Bishop(me); u; Cut(u))
        {
        uint64 sq = lsb(u);
        uint64 att = BishopAttacks(sq, occ);
        Current->att[me] |= att;

        if (BMask[sq] & King(opp))
            if (uint64 v = (Between[VarC(king, opp)][sq] & occ))
                if (Single(v))
                    Current->xray[me] |= v;
        }

    for (uint64 u = Rook(me); u; Cut(u))
        {
        uint64 sq = lsb(u);
        uint64 att = RookAttacks(sq, occ);
        Current->att[me] |= att;

        if (RMask[sq] & King(opp))
            if (uint64 v = (Between[VarC(king, opp)][sq] & occ))
                if (Single(v))
                    Current->xray[me] |= v;
        }

    for (uint64 u = Queen(me); u; Cut(u))
        {
        uint64 sq = lsb(u);
        uint64 att = QueenAttacks(sq, occ);
        Current->att[me] |= att;

        if (QMask[sq] & King(opp))
            if (uint64 v = (Between[VarC(king, opp)][sq] & occ))
                if (Single(v))
                    Current->xray[me] |= v;
        }
#undef me
#define me Black

    for (uint64 u = Knight(me); u; Cut(u))
        Current->att[me] |= NAtt[lsb(u)];

    for (uint64 u = Bishop(me); u; Cut(u))
        {
        uint64 sq = lsb(u);
        uint64 att = BishopAttacks(sq, occ);
        Current->att[me] |= att;

        if (BMask[sq] & King(opp))
            if (uint64 v = (Between[VarC(king, opp)][sq] & occ))
                if (Single(v))
                    Current->xray[me] |= v;
        }

    for (uint64 u = Rook(me); u; Cut(u))
        {
        uint64 sq = lsb(u);
        uint64 att = RookAttacks(sq, occ);
        Current->att[me] |= att;

        if (RMask[sq] & King(opp))
            if (uint64 v = (Between[VarC(king, opp)][sq] & occ))
                if (Single(v))
                    Current->xray[me] |= v;
        }

    for (uint64 u = Queen(me); u; Cut(u))
        {
        uint64 sq = lsb(u);
        uint64 att = QueenAttacks(sq, occ);
        Current->att[me] |= att;

        if (QMask[sq] & King(opp))
            if (uint64 v = (Between[VarC(king, opp)][sq] & occ))
                if (Single(v))
                    Current->xray[me] |= v;
        }
#undef me
    }

static void prt_str(char* str, int mirror)
    {
    int color, pt, i;

    color = !mirror ? WHITE : BLACK;

    for (pt = KING; pt >= PAWN; --pt)
        for (i = popcnt(bb_color_pt(color, pt)); i > 0; i--)
            *str++ = pchr[6 - pt];
    *str++ = 'v';
    color ^= 1;

    for (pt = KING; pt >= PAWN; --pt)
        for (i = popcnt(bb_color_pt(color, pt)); i > 0; i--)
            *str++ = pchr[6 - pt];
    *str++ = 0;
    }

static uint64 calc_key(int mirror)
    {
    int color, pt, i;
    uint64 key = 0;

    color = !mirror ? WHITE : BLACK;

    for (pt = PAWN; pt <= KING; ++pt)
        for (i = popcnt(bb_color_pt(color, pt)); i > 0; i--)
            key ^= psq(WHITE, pt, i - 1);
    color ^= 1;

    for (pt = PAWN; pt <= KING; ++pt)
        for (i = popcnt(bb_color_pt(color, pt)); i > 0; i--)
            key ^= psq(BLACK, pt, i - 1);

    return key;
    }

static uint64 calc_key_from_pcs(int* pcs, int mirror)
    {
    int color, pt, i;
    uint64 key = 0;

    color = !mirror ? 0 : 8;

    for (pt = PAWN; pt <= KING; ++pt)
        for (i = 0; i < pcs[color + pt]; i++)
            key ^= psq(WHITE, pt, i);
    color ^= 8;

    for (pt = PAWN; pt <= KING; ++pt)
        for (i = 0; i < pcs[color + pt]; i++)
            key ^= psq(BLACK, pt, i);

    return key;
    }

static int probe_wdl_table(int* success)
    {
    struct TBEntry* ptr;
    struct TBHashEntry* ptr2;
    uint64 idx;
    uint64 key;
    int i;
    ubyte res;
    int p[TBPIECES];

    key = calc_key(0);

    if (key == (psq(WHITE, KING, 0) ^ psq(BLACK, KING, 0)))
        return 0;

    ptr2 = TB_hash[key >> (64 - TBHASHBITS)];

    for (i = 0; i < HSHMAX; i++)
        if (ptr2[i].key == key)
            break;

    if (i == HSHMAX)
        {
        * success = 0;
        return 0;
        }

    ptr = ptr2[i].ptr;

    if (!ptr->ready)
        {
        LOCK(TB_mutex);

        if (!ptr->ready)
            {
            char str[16];
            prt_str(str, ptr->key != key);

            if (!init_table_wdl(ptr, str))
                {
                ptr2[i].key = 0ULL;
                * success = 0;
                UNLOCK(TB_mutex);
                return 0;
                }
            ptr->ready = 1;
            }
        UNLOCK(TB_mutex);
        }

    int bside, mirror, cmirror;

    if (!ptr->symmetric)
        {
        if (key != ptr->key)
            {
            cmirror = 8;
            mirror = 0x38;
            bside = (Current->turn == WHITE);
            }
        else
            {
            cmirror = mirror = 0;
            bside = !(Current->turn == WHITE);
            }
        }
    else
        {
        cmirror = Current->turn == WHITE ? 0 : 8;
        mirror = Current->turn == WHITE ? 0 : 0x38;
        bside = 0;
        }

    if (!ptr->has_pawns)
        {
        struct TBEntry_piece* entry = (struct TBEntry_piece *)ptr;
        ubyte* pc = entry->pieces[bside];

        for (i = 0; i < entry->num;)
            {
            Bitboard bb = bb_color_pt(((pc[i] ^ cmirror) >> 3), (pc[i] & 0x07));

            do
                {
                p[i++] = pop_lsb(&bb);
                } while (bb);
            }
        idx = encode_piece(entry, entry->norm[bside], p, entry->factor[bside]);
        res = decompress_pairs(entry->precomp[bside], idx);
        }
    else
        {
        struct TBEntry_pawn* entry = (struct TBEntry_pawn *)ptr;
        int k = entry->file[0].pieces[0][0] ^ cmirror;
        Bitboard bb = bb_color_pt((k >> 3), (k & 0x07));
        i = 0;

        do
            {
            p[i++] = pop_lsb(&bb) ^ mirror;
            } while (bb);
        int f = pawn_file(entry, p);
        ubyte* pc = entry->file[f].pieces[bside];

        for (; i < entry->num;)
            {
            bb = bb_color_pt(((pc[i] ^ cmirror) >> 3), (pc[i] & 0x07));

            do
                {
				if (i < 6)
					p[i++] = pop_lsb(&bb) ^ mirror;
                } while (bb);
            }
        idx = encode_pawn(entry, entry->file[f].norm[bside], p, entry->file[f].factor[bside]);
        res = decompress_pairs(entry->file[f].precomp[bside], idx);
        }

    return ((int)res) - 2;
    }

static int probe_dtz_table(int wdl, int* success)
    {
    struct TBEntry* ptr;
    uint64 idx;
    int i, res;
    int p[TBPIECES];

    uint64 key = calc_key(0);

    if (DTZ_table[0].key1 != key && DTZ_table[0].key2 != key)
        {
        for (i = 1; i < DTZ_ENTRIES; i++)
            if (DTZ_table[i].key1 == key)
                break;

        if (i < DTZ_ENTRIES)
            {
            struct DTZTableEntry table_entry = DTZ_table[i];

            for (; i > 0; i--)
                DTZ_table[i] = DTZ_table[i - 1];
            DTZ_table[0] = table_entry;
            }
        else
            {
            struct TBHashEntry* ptr2 = TB_hash[key >> (64 - TBHASHBITS)];

            for (i = 0; i < HSHMAX; i++)
                if (ptr2[i].key == key)
                    break;

            if (i == HSHMAX)
                {
                * success = 0;
                return 0;
                }
            ptr = ptr2[i].ptr;
            char str[16];
            int mirror = (ptr->key != key);
            prt_str(str, mirror);

            if (DTZ_table[DTZ_ENTRIES - 1].entry)
                free_dtz_entry(DTZ_table[DTZ_ENTRIES - 1].entry);

            for (i = DTZ_ENTRIES - 1; i > 0; i--)
                DTZ_table[i] = DTZ_table[i - 1];
            load_dtz_table(str, calc_key(mirror), calc_key(!mirror));
            }
        }

    ptr = DTZ_table[0].entry;

    if (!ptr)
        {
        * success = 0;
        return 0;
        }

    int bside, mirror, cmirror;

    if (!ptr->symmetric)
        {
        if (key != ptr->key)
            {
            cmirror = 8;
            mirror = 0x38;
            bside = (Current->turn == WHITE);
            }
        else
            {
            cmirror = mirror = 0;
            bside = !(Current->turn == WHITE);
            }
        }
    else
        {
        cmirror = Current->turn == WHITE ? 0 : 8;
        mirror = Current->turn == WHITE ? 0 : 0x38;
        bside = 0;
        }

    if (!ptr->has_pawns)
        {
        struct DTZEntry_piece* entry = (struct DTZEntry_piece *)ptr;

        if ((entry->flags & 1) != bside && !entry->symmetric)
            {
            * success = -1;
            return 0;
            }
        ubyte* pc = entry->pieces;

        for (i = 0; i < entry->num;)
            {
            Bitboard bb = bb_color_pt(((pc[i] ^ cmirror) >> 3), (pc[i] & 0x07));

            do
                {
                p[i++] = pop_lsb(&bb);
                } while (bb);
            }
        idx = encode_piece((struct TBEntry_piece *)entry, entry->norm, p, entry->factor);
        res = decompress_pairs(entry->precomp, idx);

        if (entry->flags & 2)
            res = entry->map[entry->map_idx[wdl_to_map[wdl + 2]] + res];

        if (!(entry->flags & pa_flags[wdl + 2]) || (wdl & 1))
            res *= 2;
        }
    else
        {
        struct DTZEntry_pawn* entry = (struct DTZEntry_pawn *)ptr;
        int k = entry->file[0].pieces[0] ^ cmirror;
        Bitboard bb = bb_color_pt((k >> 3), (k & 0x07));
        i = 0;

        do
            {
            p[i++] = pop_lsb(&bb) ^ mirror;
            } while (bb);
        int f = pawn_file((struct TBEntry_pawn *)entry, p);

        if ((entry->flags[f] & 1) != bside)
            {
            * success = -1;
            return 0;
            }
        ubyte* pc = entry->file[f].pieces;

        for (; i < entry->num;)
            {
            bb = bb_color_pt(((pc[i] ^ cmirror) >> 3), (pc[i] & 0x07));

            do
                {
				if (i < 6)
					p[i++] = pop_lsb(&bb) ^ mirror;
                } while (bb);
            }
        idx = encode_pawn((struct TBEntry_pawn *)entry, entry->file[f].norm, p, entry->file[f].factor);
        res = decompress_pairs(entry->file[f].precomp, idx);

        if (entry->flags[f] & 2)
            res = entry->map[entry->map_idx[f][wdl_to_map[wdl + 2]] + res];

        if (!(entry->flags[f]&pa_flags[wdl + 2]) || (wdl & 1))
            res *= 2;
        }

    return res;
    }

static int probe_ab(int alpha, int beta, int* success)
    {
    int moves[128], move, v;

    Current->mask = Piece(Current->turn ^ 1);

    if (Current->turn == White)
        gen_captures < 0, 1 > (moves);
    else
        gen_captures < 1, 1 > (moves);

    for (int* p = moves; move = (*p) &0xFFFF; p++)
        {
        int me = Current->turn;

        if (!Square(To(move)) || IsEP(move) || IsIllegal(me, move))
            continue;

        if (Current->turn == White)
            do_move <0> (move);
        else
            do_move <1> (move);
        nodes--;
        compute_pos_info();

        if (Check(Current->turn ^ 1))
            {
            if (Current->turn == White)
                undo_move <1> (move);
            else
                undo_move <0> (move);
            continue;
            }
        v = -probe_ab(-beta, -alpha, success);

        if (Current->turn == White)
            undo_move <1> (move);
        else
            undo_move <0> (move);

        if (*success == 0)
            return 0;

        if (v > alpha)
            {
            if (v >= beta)
                {
                * success = 2;
                return v;
                }
            alpha = v;
            }
        }

    v = probe_wdl_table(success);

    if (*success == 0)
        return 0;

    if (alpha >= v)
        {
        * success = 1 + (alpha > 0);
        return alpha;
        }
    else
        {
        * success = 1;
        return v;
        }
    }

int Tablebases::probe_wdl(int* success)
    {
    int v;

    * success = 1;
    evaluate();
    v = probe_ab(-2, 2, success);

    if (!Current->ep_square)
        return v;

    if (!(*success))
        return 0;

    int v1 = -3;
    int moves[128], move, me = Current->turn;
    int* p = moves;

    for (uint64 u = (Pawn(me) & PAtt[opp][Current->ep_square]); u; Cut(u))
        {
        * p = (lsb(u) << 6) | Current->ep_square | FlagEP;
        p++;
        }
    * p = 0;

    for (p = moves; move = (*p) &0xFFFF; p++)
        {
        if (IsIllegal(me, move))
            continue;

        if (Current->turn == White)
            do_move <0> (move);
        else
            do_move <1> (move);
        nodes--;
        compute_pos_info();

        if (Check(Current->turn ^ 1))
            {
            if (Current->turn == White)
                undo_move <1> (move);
            else
                undo_move <0> (move);
            continue;
            }
        int v0 = -probe_ab(-2, 2, success);

        if (Current->turn == White)
            undo_move <1> (move);
        else
            undo_move <0> (move);

        if (*success == 0)
            return 0;

        if (v0 > v1)
            v1 = v0;
        }

    if (v1 > -3)
        {
        if (v1 >= v)
            v = v1;
        else if (v == 0)
            {
            if (Check(me))
                {
                if (!me)
                    gen_evasions <0> (moves);
                else
                    gen_evasions <1> (moves);
                }
            else
                {
                Current->mask = Piece(opp);

                if (!me)
                    {
                    p = gen_captures < 0, 0 > (moves);
                    gen_quiet_moves <0> (p);
                    }
                else
                    {
                    p = gen_captures < 1, 0 > (moves);
                    gen_quiet_moves <1> (p);
                    }
                }

            for (p = moves; move = (*p) &0xFFFF; p++)
                {
                if (IsEP(move))
                    continue;

                if (IsIllegal(me, move))
                    continue;
                return v;
                }
            return v1;
            }
        }

    return v;
    }

static int probe_dtz_no_ep(int* success)
    {
    int wdl, dtz;

    evaluate();
    wdl = probe_ab(-2, 2, success);

    if (*success == 0)
        return 0;

    if (wdl == 0)
        return 0;

    if (*success == 2)
        return wdl == 2 ? 1 : 101;

    int moves[128], move, v;
    Current->mask = Piece(Current->turn ^ 1);

    if (Current->turn == White)
        {
        int*p = gen_captures < 0, 1 > (moves);
        gen_quiet_moves <0> (p);
        }
    else
        {
        int*p = gen_captures < 1, 1 > (moves);
        gen_quiet_moves <1> (p);
        }

    if (wdl > 0)
        {
        for (int* p = moves; move = (*p) &0xFFFF; p++)
            {
            int me = Current->turn;

            if (Square(From(move)) >= WhiteKnight || IsEP(move) || IsIllegal(me, move) || Square(To(move)))
                continue;

            if (Current->turn == White)
                do_move <0> (move);
            else
                do_move <1> (move);
            nodes--;
            compute_pos_info();

            if (Check(Current->turn ^ 1))
                {
                if (Current->turn == White)
                    undo_move <1> (move);
                else
                    undo_move <0> (move);
                continue;
                }
            v = -probe_ab(-2, -wdl + 1, success);

            if (Current->turn == White)
                undo_move <1> (move);
            else
                undo_move <0> (move);

            if (*success == 0)
                return 0;

            if (v == wdl)
                return v == 2 ? 1 : 101;
            }
        }

    dtz = 1 + probe_dtz_table(wdl, success);

    if (*success >= 0)
        {
        if (wdl & 1)
            dtz += 100;
        return wdl >= 0 ? dtz : -dtz;
        }

    if (wdl > 0)
        {
        int best = 0xffff;

        for (int* p = moves; move = (*p) &0xFFFF; p++)
            {
            int me = Current->turn;

            if (Square(From(move)) < WhiteKnight || IsEP(move) || IsIllegal(me, move) || Square(To(move)))
                continue;

            if (Current->turn == White)
                do_move <0> (move);
            else
                do_move <1> (move);
            nodes--;
            compute_pos_info();

            if (Check(Current->turn ^ 1))
                {
                if (Current->turn == White)
                    undo_move <1> (move);
                else
                    undo_move <0> (move);
                continue;
                }
            v = -Tablebases::probe_dtz(success);

            if (Current->turn == White)
                undo_move <1> (move);
            else
                undo_move <0> (move);

            if (*success == 0)
                return 0;

            if (v > 0 && v + 1 < best)
                best = v + 1;
            }
        return best;
        }
    else
        {
        int best = -1;

        for (int* p = moves; move = (*p) &0xFFFF; p++)
            {
            int me = Current->turn;

            if (IsIllegal(me, move))
                continue;

            if (Current->turn == White)
                do_move <0> (move);
            else
                do_move <1> (move);
            nodes--;
            compute_pos_info();

            if (Check(Current->turn ^ 1))
                {
                if (Current->turn == White)
                    undo_move <1> (move);
                else
                    undo_move <0> (move);
                continue;
                }

            if (!Current->ply)
                {
                if (wdl == -2)
                    v = -1;
                else
                    {
                    v = probe_ab(1, 2, success);
                    v = (v == 2) ? 0 : -101;
                    }
                }
            else
                {
                v = -Tablebases::probe_dtz(success) - 1;
                }

            if (Current->turn == White)
                undo_move <1> (move);
            else
                undo_move <0> (move);

            if (*success == 0)
                return 0;

            if (v < best)
                best = v;
            }
        return best;
        }
    }

static int wdl_to_dtz [] =
    {
    -1, -101, 0, 101, 1
    };

int Tablebases::probe_dtz(int* success)
    {
    * success = 1;
    int v = probe_dtz_no_ep(success);

    if (!Current->ep_square)
        return v;

    if (*success == 0)
        return 0;

    int v1 = -3;
    int moves[128], move, me = Current->turn;
    int* p = moves;

    for (uint64 u = (Pawn(me) & PAtt[opp][Current->ep_square]); u; Cut(u))
        {
        * p = (lsb(u) << 6) | Current->ep_square | FlagEP;
        p++;
        }
    * p = 0;

    for (p = moves; move = (*p) &0xFFFF; p++)
        {
        if (IsIllegal(me, move))
            continue;

        if (Current->turn == White)
            do_move <0> (move);
        else
            do_move <1> (move);
        nodes--;
        compute_pos_info();

        if (Check(Current->turn ^ 1))
            {
            if (Current->turn == White)
                undo_move <1> (move);
            else
                undo_move <0> (move);
            continue;
            }
        int v0 = -probe_ab(-2, 2, success);

        if (Current->turn == White)
            undo_move <1> (move);
        else
            undo_move <0> (move);

        if (*success == 0)
            return 0;

        if (v0 > v1)
            v1 = v0;
        }

    if (v1 > -3)
        {
        v1 = wdl_to_dtz[v1 + 2];

        if (v < -100)
            {
            if (v1 >= 0)
                v = v1;
            }
        else if (v < 0)
            {
            if (v1 >= 0 || v1 < -100)
                v = v1;
            }
        else if (v > 100)
            {
            if (v1 > 0)
                v = v1;
            }
        else if (v > 0)
            {
            if (v1 == 1)
                v = v1;
            }
        else if (v1 >= 0)
            {
            v = v1;
            }
        else
            {
            if (Check(me))
                {
                if (!me)
                    gen_evasions <0> (moves);
                else
                    gen_evasions <1> (moves);
                }
            else
                {
                Current->mask = Piece(opp);

                if (!me)
                    {
                    p = gen_captures < 0, 0 > (moves);
                    gen_quiet_moves <0> (p);
                    }
                else
                    {
                    p = gen_captures < 1, 0 > (moves);
                    gen_quiet_moves <1> (p);
                    }
                }

            for (p = moves; move = (*p) &0xFFFF; p++)
                {
                if (IsEP(move))
                    continue;

                if (IsIllegal(me, move))
                    continue;
                return v;
                }
            return v1;
            }
        }
    return v;
    }