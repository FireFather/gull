#ifndef FUNCTION_H_INCLUDED
#define FUNCTION_H_INCLUDED

__forceinline int lsb(uint64 x);
__forceinline int msb(uint64 x);
__forceinline int popcnt(uint64 x);
template <bool HPopCnt> __forceinline int popcount(uint64 x);
static __forceinline int MinF(int x, int y);
static __forceinline int MaxF(int x, int y);
static __forceinline double MinF(double x, double y);
static __forceinline double MaxF(double x, double y);

uint64 BMagicAttacks(int i, uint64 occ);
uint64 RMagicAttacks(int i, uint64 occ);

static uint16 rand16();
static uint64 random();

void init_misc();
void init_magic();
void gen_kpk();
void init_eval();
void init_pst();

static void calc_material(int index);
void init_material();

void init_search(int clear_hash);
void setup_board();
void get_board(const char fen[]);
void init_hash();
__forceinline GEntry* probe_hash();
__forceinline GPVEntry* probe_pv_hash();
void move_to_string(int move, char string[]);
int move_from_string(char string[]);
void pick_pv();
template <int me> static int draw_in_pv();
template <int me> void do_move(int move);
template <int me> void undo_move(int move);
void do_null();
void undo_null();
template <int me> static int krbkrx();
template <int me> static int kpkx();
template <int me> static int knpkx();
template <int me> static int krpkrx();
template <int me> static int krpkbx();
template <int me> static int kqkp();
template <int me> static int kqkrpx();
template <int me> static int krkpx();
template <int me> static int krppkrpx();
template <int me> static int krpppkrppx();
template <int me> static int kbpkbx();
template <int me> static int kbpknx();
template <int me> static int kbppkbx();
template <int me> static int krppkrx();
template <int me, bool HPopCnt> __forceinline static void eval_pawns(GPawnEntry* PawnEntry, GPawnEvalInfo &PEI);
template <bool HPopCnt> static void eval_pawn_structure(GPawnEntry* PawnEntry);
template <int me, bool HPopCnt> __forceinline static void eval_queens(GEvalInfo &EI);
template <int me, bool HPopCnt> __forceinline static void eval_rooks(GEvalInfo &EI);
template <int me, bool HPopCnt> __forceinline static void eval_bishops(GEvalInfo &EI);
template <int me, bool HPopCnt> __forceinline static void eval_knights(GEvalInfo &EI);
template <int me, bool HPopCnt> __forceinline static void eval_king(GEvalInfo &EI);
template <int me, bool HPopCnt> __forceinline static void eval_passer(GEvalInfo &EI);
template <int me, bool HPopCnt> __forceinline static void eval_pieces(GEvalInfo &EI);
template <int me, bool HPopCnt> static void eval_endgame(GEvalInfo &EI);
template <bool HPopCnt> static void eval_unusual_material(GEvalInfo &EI);
template <bool HPopCnt> static void evaluation();
__forceinline void evaluate();
template <int me> int is_legal(int move);
template <int me> static int is_check(int move);
void hash_high(int value, int depth);
void hash_low(int move, int value, int depth);
void hash_exact(int move, int value, int depth, int exclusion, int ex_depth, int knodes);
template <bool pv> __forceinline static int extension(int move, int depth);

void sort(int* start, int* finish);
void sort_moves(int* start, int* finish);
__forceinline int pick_move();

template <int me> void gen_next_moves();
template <int me, bool root> static int get_move();
template <int me> static int see(int move, int margin);

template <int me, bool up> static int* gen_captures(int* list);
template <int me> static int* gen_evasions(int* list);
static void mark_evasions(int* list);
template <int me> static int* gen_quiet_moves(int* list);
template <int me> static int* gen_checks(int* list);
template <int me> static int* gen_delta_moves(int* list);
template <int me> static int singular_extension(int ext, int prev_ext, int margin_one, int margin_two, int depth, int killer);
template <int me> __forceinline static void capture_margin(int alpha, int &score);
template <int me, bool pv> static int q_search(int alpha, int beta, int depth, int flags);
template <int me, bool pv> static int q_evasion(int alpha, int beta, int depth, int flags);
template <int me, bool exclusion> int search(int beta, int depth, int flags);
template <int me, bool exclusion> static int search_evasion(int beta, int depth, int flags);
template <int me, bool root> int pv_search(int alpha, int beta, int depth, int flags);
template <int me> static void root();
template <int me> static int multipv(int depth);
void send_pv(int depth, int alpha, int beta, int score);
void send_multipv(int depth, int curr_number);
void send_best_move();
void get_position(char string[]);
void get_time_limit(char string[]);
sint64 get_time();
int time_to_stop(GSearchInfo* SI, int time, int searching);
void check_time(int searching);
void check_time(int time, int searching);
void check_state();
int input();
void uci();
uint64 GetClock();

void Bench();
static void CreateBenchLog();

void send_position(GPos* Pos);
void retrieve_board(GPos* Pos);
void retrieve_position(GPos* Pos, int copy_stack);
void halt_all(GSP* Sp, int locked);
void halt_all(int from, int to);
void init_sp(GSP* Sp, int alpha, int beta, int depth, int pv, int singular, int height);
template <int me> static int smp_search(GSP* Sp);
void init_shared();
void init();
HANDLE CreateChildProcess(int child_id);
void init_proc();
void init_sys(int argc, char* argv[]);
int main(int argc, char* argv[]);

#endif