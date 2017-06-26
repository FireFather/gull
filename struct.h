#ifndef STRUCT_H_INCLUDED
#define STRUCT_H_INCLUDED

#include <setjmp.h> 

typedef struct
    {
    uint64 bb[16];//128
    uint8 square[64];//64
    } GBoard;//192
typedef struct
    {
    uint64 key;//8
    uint64 pawn_key;//8
    uint16 move;//2
    uint8 turn;//1
    uint8 castle_flags;//1
    uint8 ply;//1
    uint8 ep_square;//1
    uint8 piece;//1
    uint8 capture;//1
    uint8 square[64];//64
    int pst;//4
    int material;//4	
    } GPosData;//96
typedef struct
    {
    uint64 key;//8
	uint64 pawn_key;//8
	uint64 eval_key;//8
	uint64 att[2];//16
	uint64 patt[2];//16
	uint64 passer;//8	
	uint64 xray[2];//16
	uint64 pin[2];//16
	uint64 threat;//8
	uint64 mask;//8
    uint8 turn;//1
	uint8 castle_flags;//1
	uint8 ply;//1
	uint8 ep_square;//1
	uint8 capture;//1
	uint8 gen_flags;//1
	uint8 piece;//1
	uint8 stage;//1
	uint8 mul;//1
	uint8 piece_nb;//1
    sint16 score;//2
    uint16 move;//2
    uint16 killer[3];//6
    uint16 ref[2];//4
    int best;//4
    int material;//4
	int pst;//4
    int margin;//4					
    int *start;//8
    int *current;//8
    int moves[256];//1024	
    } GData;//1192
typedef struct
    {
    uint32 key;//4
    uint16 date;//2
    uint16 move;//2
    sint16 low;//2
    sint16 high;//2
    uint16 flags;//2
    uint8 low_depth;//1
    uint8 high_depth;//1
    } GEntry;//16
typedef struct
    {
    uint64 key;//8
    sint16 shelter[2];//4
    uint8 passer[2];//2
    uint8 draw[2];//2
    int score;//4
    } GPawnEntry;//20
typedef struct
    {
    sint16 score;//2
    uint8 phase;//1
    uint8 flags;//1	
    uint8 mul[2];//2
    uint8 pieces[2];//2
    } GMaterial;//8
typedef struct
    {
    uint16 ref[2];//4
    uint16 check_ref[2];//4
    } GRef;//8
typedef struct
    {
    int bad;//4
    int change;//4
    int singular;//4
    int early;//4
    int fail_low;//4
    int fail_high;//4
    } GSearchInfo;//24
typedef struct
    {
    uint32 key;//4
    uint16 date;//2
    uint16 move;//2
    sint16 value;//2
    sint16 exclusion;//2
    uint8 depth;//1
    uint8 ex_depth;//1
    int knodes;//4
    int ply;//4
    } GPVEntry;//34
typedef struct
    {
    GPosData position[1];//100
    uint64 stack[100];//800
    uint16 killer[16][2];//64
    int sp;//4
    int date;//4
    } GPos;//972
typedef struct
    {
    volatile uint16 move;//2
    volatile uint8 reduced_depth;//1
    volatile uint8 research_depth;//1
    volatile uint8 stage;//1
    volatile uint8 ext;//1
    volatile uint8 id;//1
    volatile uint8 flags;//1
    } GMove;//8
typedef struct
    {
    volatile LONG lock;//4
    volatile int claimed;//4
    volatile int active;//4
    volatile int finished;//4
    volatile int pv;//4
    volatile int move_number;//4
    volatile int current;//4
    volatile int depth;//4
    volatile int alpha;//4
    volatile int beta;//4
    volatile int singular;//4
    volatile int split;//4
    volatile int best_move;//4
    volatile int height;//4
    GMove move[128];//1024	
    jmp_buf jump;//?
    GPos position[1];//972
    } GSP;//1172

typedef struct
    {
    volatile long long nodes;//8
    volatile long long tb_hits;//8
    volatile long long active_sp;//8
    volatile long long searching;//8

#ifndef W32_BUILD
    volatile long long stop;//8
    volatile long long fail_high;//8
#else
    volatile long stop;//4
    volatile long fail_high;//4
#endif

	volatile sint64 hash_size;
	volatile int PrN;
    GSP sp[MaxSplitPoints];//64 * 1172 = 75008
    } GSMPI;//75056

typedef struct
    {
    int score;//4
    int king_w;//4
    int king_b;//4
    int mul;//4
    uint64 occ;//8
    uint64 area_w;//8
    uint64 area_b;//8
    uint64 free_w;//8
    uint64 free_b;//8
    uint32 king_att_w;//4
    uint32 king_att_b;//4
    GPawnEntry* PawnEntry;//20
    GMaterial* material;//8	
    } GEvalInfo;//92

typedef struct
    {
    int king_w;//4
    int king_b;//4
    int score;//4	
    uint64 patt_w;//8
    uint64 patt_b;//8
    uint64 double_att_w;//8
    uint64 double_att_b;//8
    } GPawnEvalInfo;//44

#endif