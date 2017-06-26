#ifndef TBPROBE_H
#define TBPROBE_H

namespace Tablebases
    {
    extern int TBLargest;
    void init(const std::string & path);
    int probe_wdl(int*success);
    int probe_dtz(int*success);
    }

int probe_win(int* success)
    {
    if (Current->piece_nb > Tablebases::TBLargest)
        {
        * success = 0;
        return 0;
        }
    int score = 0;

    if (Current->piece_nb + (int)(Current - Data) >= 125)
        {
        * success = 0;
        return 0;
        }
    int value = Tablebases::probe_wdl(success);

    if (*success)
        {

#ifndef W32_BUILD
        InterlockedAdd64(&Smpi->tb_hits, (long long)1);
#else
        Smpi->tb_hits++;
#endif

        if (Abs(value) <= 1)
            score = 0;

        else if (value < -1)
            score = -WdlValue + (int)(Current - Data) + (Current->piece_nb << 7);

        else if (value > 1)
            score = WdlValue - (int)(Current - Data) - (Current->piece_nb << 7);
        hash_low(0, score, 127);
        hash_high(score, 127);
        }
    return score;
    }

int probe_distance(int* success)
    {
    if (Current->piece_nb > Tablebases::TBLargest)
        {
        * success = 0;
        return 0;
        }
    int score = 0;

    if (Current->piece_nb + (int)(Current - Data) >= 125)
        {
        * success = 0;
        return 0;
        }
    int value = Tablebases::probe_dtz(success);

    if (*success)
        {

#ifndef W32_BUILD
        InterlockedAdd64(&Smpi->tb_hits, (long long)1);
#else
        Smpi->tb_hits++;
#endif

        if (value && Abs(value) <= 100)
            {
            if (Current->ply + Abs(value) > 100)
                return 0;

            if (Current->ply + Abs(value) == 100)
                return Sgn(value);
            }

        if (!value || Abs(value) > 100)
            score = 0;
        else
            {
            if (value < 0)
                score = -DtzValue + 500 - value - (200 * F(Current->ply));

            else if (value > 0)
                score = DtzValue - 500 - value + (200*F(Current->ply));
            }
        }
    return score;
    }
#endif