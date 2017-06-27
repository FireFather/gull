# seagull
chess engine based on Gull 3 by Thinkingalot (Vadim Demichev)

https://sourceforge.net/projects/gullchess/

with support for syzygy tablebases

-source code cleaned up, simplified, and broken up into various source and header files 

-a couple of VS code analysis fixes (ex: gen_kpk() was causing stack exceed error) 
so it's now implemented using uint8 Kpk_gen[2][64][64][64] 

-compiler warnings resolved up to level 4 

-benchmark and perft utilities added
(type 'bench' and engine will write a date-stamped text file with results)

-small change to phase/piece value calculation adding 3-5 elo: 

Settings = Gauntlet/128MB/1000ms+100ms/M 500cp for 6 moves, D 120 moves/PGN:10000.pgn(10000)	
(Avg game length = 13.252 sec)	

1 Seagull 138 : 3002 9795/38207 (+9795,=19150,-9262) 50.7 %          
2 Gull 3      : 2998 9262/38207 (+9262,=19150,-9795) 49.3 %

games = 38207	
win% = 0.506975	
elo = +4.84715	
los = 0.999926

I've included the Visual Studio project files (Seagull.vcxproj, etc)

Norman Schmidt
firefather@telenet.be
6/26/2017
