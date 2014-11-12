
all: main
.PHONY : all

main: main.cpp netmap.hpp common.hpp ether.hpp
	c++ -DDEBUG -DPOLL -std=c++11 -Wall ./main.cpp -o slb
.PHONY : main

getinfo: getinfo.c netmap.hpp common.hpp ether.hpp
	cc -DDEBUG -Wall ./getinfo.c -o getinfo
.PHONY : getinfo

clean:
	rm -rf slb getinfo frame_dump_select frame_dump_poll
.PHONY : clean

frame_dump: frame_dump_p frame_dump_s
# sample code
frame_dump_p: frame_dump.cpp netmap.hpp common.hpp ether.hpp
	c++ -DDEBUG -DPOLL -std=c++11 -Wall ./frame_dump.cpp -o frame_dump_poll
.PHONY : frame_dump
# sample code
frame_dump_s: frame_dump.cpp netmap.hpp common.hpp ether.hpp
	c++ -DDEBUG -std=c++11 -Wall ./frame_dump.cpp -o frame_dump_select
.PHONY : frame_dump

