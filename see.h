#ifndef SEE_H_INCLUDED
#define SEE_H_INCLUDED

template <int me> static int see(int move, int margin)
{
	int from = From(move);
	int to = To(move);
	int piece = SeeValue[Square(from)];
	int capture = SeeValue[Square(to)];
	int delta = piece - capture;
	int sq = 0, pos = 0;

	uint64 clear = 0, def = 0, att = 0, occ = 0, r_area = 0, b_area = 0;
	uint64 r_slider_att = 0, b_slider_att = 0, r_slider_def = 0, b_slider_def = 0;
	uint64 u = 0, new_att = 0, my_bishop = 0, opp_bishop = 0;

	if (delta <= -margin)
		return 1;

	if (piece == SeeValue[WhiteKing])
		return 1;

	if (Current->xray[me] & Bit(from))
		return 1;

	if (T(Current->pin[me] & Bit(from)) && piece <= SeeValue[WhiteDark])
		return 1;

	if (piece > (SeeValue[WhiteKing] >> 1))
		return 1;

	if (IsEP(move))
		return 1;

	if (F(Current->att[opp] & Bit(to)))
		return 1;
	att = PAtt[me][to] & Pawn(opp);

	if (T(att) && delta + margin > SeeValue[WhitePawn])
		return 0;
	clear = ~Bit(from);
	def = PAtt[opp][to] & Pawn(me) & clear;

	if (T(def) && delta + SeeValue[WhitePawn] + margin <= 0)
		return 1;
	att |= NAtt[to] & Knight(opp);

	if (T(att) && delta > SeeValue[WhiteDark] - margin)
		return 0;
	occ = PieceAll & clear;
	b_area = BishopAttacks(to, occ);
	opp_bishop = Bishop(opp);

	if (delta > SeeValue[IDark(me)] - margin)
		if (b_area & opp_bishop)
			return 0;
	my_bishop = Bishop(me);
	b_slider_att = BMask[to] & (opp_bishop | Queen(opp));
	r_slider_att = RMask[to] & Major(opp);
	b_slider_def = BMask[to] & (my_bishop | Queen(me)) & clear;
	r_slider_def = RMask[to] & Major(me) & clear;
	att |= (b_slider_att & b_area);
	def |= NAtt[to] & Knight(me) & clear;
	r_area = RookAttacks(to, occ);
	att |= (r_slider_att & r_area);
	def |= (b_slider_def & b_area);
	def |= (r_slider_def & r_area);
	att |= SArea[to] & King(opp);
	def |= SArea[to] & King(me) & clear;

	while (true)
	{
		if (u = (att & Pawn(opp)))
		{
			capture -= piece;
			piece = SeeValue[WhitePawn];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);

			for (new_att = FullLine[to][sq] & b_slider_att & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(att, pos);
					break;
				}
			}
		}
		else if (u = (att & Knight(opp)))
		{
			capture -= piece;
			piece = SeeValue[WhiteKnight];
			att ^= (~(u - 1)) & u;
		}
		else if (u = (att & opp_bishop))
		{
			capture -= piece;
			piece = SeeValue[WhiteDark];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);

			for (new_att = FullLine[to][sq] & b_slider_att & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(att, pos);
					break;
				}
			}
		}
		else if (u = (att & Rook(opp)))
		{
			capture -= piece;
			piece = SeeValue[WhiteRook];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);

			for (new_att = FullLine[to][sq] & r_slider_att & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(att, pos);
					break;
				}
			}
		}
		else if (u = (att & Queen(opp)))
		{
			capture -= piece;
			piece = SeeValue[WhiteQueen];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);

			for (new_att = FullLine[to][sq] & (r_slider_att | b_slider_att) & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(att, pos);
					break;
				}
			}
		}
		else if (u = (att & King(opp)))
		{
			capture -= piece;
			piece = SeeValue[WhiteKing];
		}
		else
			return 1;

		if (capture < -(SeeValue[WhiteKing] >> 1))
			return 0;

		if (piece + capture < margin)
			return 0;

		if (u = (def & Pawn(me)))
		{
			capture += piece;
			piece = SeeValue[WhitePawn];
			sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);

			for (new_att = FullLine[to][sq] & b_slider_def & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(def, pos);
					break;
				}
			}
		}
		else if (u = (def & Knight(me)))
		{
			capture += piece;
			piece = SeeValue[WhiteKnight];
			def ^= (~(u - 1)) & u;
		}
		else if (u = (def & my_bishop))
		{
			capture += piece;
			piece = SeeValue[WhiteDark];
			sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);

			for (new_att = FullLine[to][sq] & b_slider_def & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(def, pos);
					break;
				}
			}
		}
		else if (u = (def & Rook(me)))
		{
			capture += piece;
			piece = SeeValue[WhiteRook];
			sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);

			for (new_att = FullLine[to][sq] & r_slider_def & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(def, pos);
					break;
				}
			}
		}
		else if (u = (def & Queen(me)))
		{
			capture += piece;
			piece = SeeValue[WhiteQueen];
			sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);

			for (new_att = FullLine[to][sq] & (r_slider_def | b_slider_def) & occ &(~att); T(new_att); Cut(new_att))
			{
				pos = lsb(new_att);

				if (F(Between[to][pos] & occ))
				{
					Add(def, pos);
					break;
				}
			}
		}
		else if (u = (def & King(me)))
		{
			capture += piece;
			piece = SeeValue[WhiteKing];
		}
		else
			return 0;

		if (capture > (SeeValue[WhiteKing] >> 1))
			return 1;

		if (capture - piece >= margin)
			return 1;
	}
}

#endif