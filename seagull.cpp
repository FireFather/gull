#include <windows.h>
#include <iostream>
#include <nmmintrin.h>

#include "def.h"
#include "macro.h"
#include "const.h"
#include "struct.h"
#include "function.h"
#include "seagull.h"
#include "bit.h"
#include "eval.h"
#include "search.h"
#include "endgame.h"
#include "move.h"
#include "movegen.h"
#include "smp.h"
#include "see.h"
#include "time.h"
#include "tbprobe.cpp"

using namespace std;

int main(int argc, char* argv[])
{
	init_sys(argc, argv);
	init_proc();

	while (true)
		uci();
	return EXIT_SUCCESS;
}
