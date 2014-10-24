
all: main
.PHONY : all

main: main.cpp netmap.hpp common.hpp ether.hpp
	c++ -DDEBUG -DPOLL -std=c++11 -Wall ./main.cpp -o slb
.PHONY : main

clean:
	rm -rf main
.PHONY : clean
