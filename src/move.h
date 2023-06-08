#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

__declspec(align(64)) GPawnEntry PawnHash[pawn_hash_size];

template <int me> void do_move(int move)
{
	GEntry* Entry = NULL;
	GPawnEntry* PawnEntry = NULL;
	int from = 0, to = 0, piece = 0, capture = 0;
	GData* Next;
	uint64 u = 0, mask_from = 0, mask_to = 0;

	to = To(move);
	Next = Current + 1;
	Next->ep_square = 0;
	capture = Square(to);

	if (F(capture))
	{
		Next->capture = 0;
		goto non_capture;
	}
	from = From(move);
	piece = Square(from);
	Next->turn = opp;
	Next->capture = capture;
	Next->piece_nb = Current->piece_nb - 1;
	Square(from) = 0;
	Square(to) = piece;
	Next->piece = piece;
	mask_from = Bit(from);
	mask_to = Bit(to);
	BB(piece) ^= mask_from;
	Piece(me) ^= mask_from;
	BB(capture) ^= mask_to;
	Piece(opp) ^= mask_to;
	BB(piece) |= mask_to;
	Piece(me) |= mask_to;
	Next->castle_flags = Current->castle_flags & UpdateCastling[to] & UpdateCastling[from];
	Next->pst = Current->pst + Pst(piece, to) - Pst(piece, from) - Pst(capture, to);
	Next->key = Current->key ^ PieceKey[piece][from] ^ PieceKey[piece][to] ^ PieceKey[capture][to]
		^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];

	if (capture != IPawn(opp))
		Next->pawn_key = Current->pawn_key ^ CastleKey[Current->castle_flags]
		^ CastleKey[Next->castle_flags];
	else
		Next->pawn_key = Current->pawn_key ^ PieceKey[IPawn(opp)][to] ^ CastleKey[Current->castle_flags]
		^ CastleKey[Next->castle_flags];
	Next->material = Current->material - MatCode[capture];

	if (T(Current->material & FlagUnusualMaterial) && capture >= WhiteKnight)
	{
		if (popcnt(BB(WhiteQueen)) <= 2 && popcnt(BB(BlackQueen)) <= 2)
		{
			if (popcnt(BB(WhiteLight)) <= 1 && popcnt(BB(BlackLight)) <= 1 && popcnt(BB(WhiteKnight)) <= 2
				&& popcnt(BB(BlackKnight)) <= 2 && popcnt(BB(WhiteRook)) <= 2 && popcnt(BB(BlackRook)) <= 2)
				Next->material ^= FlagUnusualMaterial;
		}
	}

	if (piece == IPawn(me))
	{
		Next->pawn_key ^= PieceKey[IPawn(me)][from] ^ PieceKey[piece][to];

		if (IsPromotion(move))
		{
			piece = Promotion(move, me);
			Square(to) = piece;
			Next->material += MatCode[piece] - MatCode[IPawn(me)];

			if (piece < WhiteRook)
			{
				if (piece >= WhiteLight && T(BB(piece)))
					Next->material |= FlagUnusualMaterial;

				if (Multiple(BB(piece)))
					Next->material |= FlagUnusualMaterial;
			}

			else if (Multiple(BB(piece)))
				Next->material |= FlagUnusualMaterial;
			Pawn(me) ^= mask_to;
			BB(piece) |= mask_to;
			Next->pst += Pst(piece, to) - Pst(IPawn(me), to);
			Next->key ^= PieceKey[piece][to] ^ PieceKey[IPawn(me)][to];
			Next->pawn_key ^= PieceKey[IPawn(me)][to];
		}
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
		prefetch((char *)PawnEntry, _MM_HINT_NTA);
	}
	else if (piece >= WhiteKing)
	{
		Next->pawn_key ^= PieceKey[piece][from] ^ PieceKey[piece][to];
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
		prefetch((char *)PawnEntry, _MM_HINT_NTA);
	}
	else if (capture < WhiteKnight)
	{
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
		prefetch((char *)PawnEntry, _MM_HINT_NTA);
	}

	if (F(Next->material & FlagUnusualMaterial))
		prefetch((char *)(Material + Next->material), _MM_HINT_NTA);

	if (Current->ep_square)
		Next->key ^= EPKey[File(Current->ep_square)];
	Next->turn = Current->turn ^ 1;
	Next->key ^= TurnKey;
	Entry = Hash + (High32(Next->key) & hash_mask);
	prefetch((char *)Entry, _MM_HINT_NTA);
	Next->ply = 0;
	goto finish;
non_capture:
	from = From(move);
	Next->ply = Current->ply + 1;
	piece = Square(from);
	Square(from) = 0;
	Square(to) = piece;
	Next->piece = piece;
	mask_from = Bit(from);
	mask_to = Bit(to);
	BB(piece) ^= mask_from;
	Piece(me) ^= mask_from;
	BB(piece) |= mask_to;
	Piece(me) |= mask_to;
	Next->castle_flags = Current->castle_flags & UpdateCastling[to] & UpdateCastling[from];
	Next->piece_nb = Current->piece_nb;
	Next->pst = Current->pst + Pst(piece, to) - Pst(piece, from);
	Next->key = Current->key ^ PieceKey[piece][to] ^ PieceKey[piece][from] ^ CastleKey[Current->castle_flags]
		^ CastleKey[Next->castle_flags];
	Next->material = Current->material;

	if (piece == IPawn(me))
	{
		Next->ply = 0;
		Next->pawn_key =
			Current->pawn_key ^ PieceKey[IPawn(me)][to] ^ PieceKey[IPawn(me)][from] ^ CastleKey[Current->castle_flags]
			^ CastleKey[Next->castle_flags];

		if (IsEP(move))
		{
			Square(to ^ 8) = 0;
			u = Bit(to ^ 8);
			Next->key ^= PieceKey[IPawn(opp)][to ^ 8];
			Next->pawn_key ^= PieceKey[IPawn(opp)][to ^ 8];
			Next->pst -= Pst(IPawn(opp), to ^ 8);
			Pawn(opp) &= ~u;
			Piece(opp) &= ~u;
			Next->material -= MatCode[IPawn(opp)];
			Next->piece_nb--;
		}
		else if (IsPromotion(move))
		{
			piece = Promotion(move, me);
			Square(to) = piece;
			Next->material += MatCode[piece] - MatCode[IPawn(me)];

			if (piece < WhiteRook)
			{
				if (piece >= WhiteLight && T(BB(piece)))
					Next->material |= FlagUnusualMaterial;

				if (Multiple(BB(piece)))
					Next->material |= FlagUnusualMaterial;
			}

			else if (Multiple(BB(piece)))
				Next->material |= FlagUnusualMaterial;
			Pawn(me) ^= mask_to;
			BB(piece) |= mask_to;
			Next->pst += Pst(piece, to) - Pst(IPawn(me), to);
			Next->key ^= PieceKey[piece][to] ^ PieceKey[IPawn(me)][to];
			Next->pawn_key ^= PieceKey[IPawn(me)][to];
		}
		else if ((to ^ from) == 16)
		{
			if (PAtt[me][(to + from) >> 1] & Pawn(opp))
			{
				Next->ep_square = (to + from) >> 1;
				Next->key ^= EPKey[File(Next->ep_square)];
			}
		}
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
		prefetch((char *)PawnEntry, _MM_HINT_NTA);
	}
	else
	{
		if (piece < WhiteKing)
			Next->pawn_key = Current->pawn_key ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
		else
		{
			Next->pawn_key =
				Current->pawn_key ^ PieceKey[piece][to] ^ PieceKey[piece][from] ^ CastleKey[Current->castle_flags]
				^ CastleKey[Next->castle_flags];
			PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
			prefetch((char *)PawnEntry, _MM_HINT_NTA);
		}

		if (IsCastling(move))
		{
			int rold = 0, rnew = 0;
			Next->ply = 0;

			if (to == 6)
			{
				rold = 7;
				rnew = 5;
			}
			else if (to == 2)
			{
				rold = 0;
				rnew = 3;
			}
			else if (to == 62)
			{
				rold = 63;
				rnew = 61;
			}
			else if (to == 58)
			{
				rold = 56;
				rnew = 59;
			}
			Add(mask_to, rnew);
			Square(rold) = 0;
			Square(rnew) = IRook(me);
			BB(IRook(me)) ^= Bit(rold);
			Piece(me) ^= Bit(rold);
			BB(IRook(me)) |= Bit(rnew);
			Piece(me) |= Bit(rnew);
			Next->pst += Pst(IRook(me), rnew) - Pst(IRook(me), rold);
			Next->key ^= PieceKey[IRook(me)][rnew] ^ PieceKey[IRook(me)][rold];
		}
	}

	if (Current->ep_square)
		Next->key ^= EPKey[File(Current->ep_square)];
	Next->turn = opp;
	Next->key ^= TurnKey;
	Entry = Hash + (High32(Next->key) & hash_mask);
	prefetch((char *)Entry, _MM_HINT_NTA);
finish:
	sp++;
	Stack[sp] = Next->key;
	Next->move = move;
	Next->gen_flags = 0;
	Current++;
	nodes++;
}

template <int me> void undo_move(int move)
{
	int piece = 0;
	int from = From(move);
	int to = To(move);

	if (IsPromotion(move))
	{
		BB(Square(to)) ^= Bit(to);
		piece = IPawn(me);
	}
	else
		piece = Square(to);
	Square(from) = piece;
	BB(piece) |= Bit(from);
	Piece(me) |= Bit(from);
	BB(piece) &= ~Bit(to);
	Piece(me) ^= Bit(to);
	Square(to) = Current->capture;

	if (Current->capture)
	{
		BB(Current->capture) |= Bit(to);
		Piece(opp) |= Bit(to);
	}
	else
	{
		if (IsCastling(move))
		{
			int rold = 0, rnew = 0;

			if (to == 6)
			{
				rold = 7;
				rnew = 5;
			}
			else if (to == 2)
			{
				rold = 0;
				rnew = 3;
			}
			else if (to == 62)
			{
				rold = 63;
				rnew = 61;
			}
			else if (to == 58)
			{
				rold = 56;
				rnew = 59;
			}
			Square(rnew) = 0;
			Square(rold) = IRook(me);
			Rook(me) ^= Bit(rnew);
			Piece(me) ^= Bit(rnew);
			Rook(me) |= Bit(rold);
			Piece(me) |= Bit(rold);
		}
		else if (IsEP(move))
		{
			to = to ^ 8;
			piece = IPawn(opp);
			Square(to) = piece;
			Piece(opp) |= Bit(to);
			Pawn(opp) |= Bit(to);
		}
	}
	Current--;
	sp--;
}

#endif