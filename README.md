# seagull
chess engine based on Gull 3 by Thinkingalot (Vadim Demichev)

https://sourceforge.net/projects/gullchess/

- source code cleaned up, simplified, and broken up into various source and header files 

- a couple of VS code analysis fixes (ex: gen_kpk() was causing stack exceed error) 

- compiler warnings resolved up to level 4 

- benchmark utility added
(type 'bench' and engine will write a date-stamped text file with results)

- small change to phase/piece value calculation adding 3-5 elo: 

- support for syzygy tablebases

![alt tag](https://raw.githubusercontent.com/FireFather/gull/master/logos/gull_7.bmp)
![alt tag](https://raw.githubusercontent.com/FireFather/gull/master/logos/gull_8.bmp)
![alt tag](https://raw.githubusercontent.com/FireFather/gull/master/logos/gull_9.bmp)
![alt tag](https://raw.githubusercontent.com/FireFather/gull/master/logos/gull_2.bmp)

Settings = Gauntlet/128MB/1000ms+100ms/M 500cp for 6 moves, D 120 moves/PGN:10000.pgn(10000)	
(Avg game length = 13.252 sec)	

| engine         | games    | win      | loss     | draw      | timeouts  | win%      | elo        | los        
| :------------: | :------: | :------: | :------: | :------:  | :------:  | :------:  | :--------: | :------:  
| Seagull 138    | 38207    | +9795    | -9262    | =19150    | 0         | 50.7 %    | +4.85	elo  | 100%
| Gull 3         | 38207    | +9262    | -9795    | =9262     | 0         | 49.3%     | -4.85 elo  | 0%


games = 38207	
win% = 0.506975	
elo = +4.84715	
los = 0.999926

I've included the Visual Studio project files (Seagull.vcxproj, etc)

Norman Schmidt
firefather@telenet.be
6/26/2017
