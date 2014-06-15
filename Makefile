CPP = g++
CPPFLAGS= -std=c++11 -W -Wall -lbz2 -lz -ggdb -O2

all: test_splitter test_period repeat_packer

bit_tools.o: bit_tools.h bit_tools.cpp
	$(CPP) $(CPPFLAGS) bit_tools.cpp -c -o bit_tools.o

splitter.o: splitter.h splitter.cpp bit_tools.h
	$(CPP) $(CPPFLAGS) splitter.cpp -c -o splitter.o

test_splitter: test_splitter.cpp splitter.o bit_tools.o
	$(CPP) $(CPPFLAGS) test_splitter.cpp splitter.o bit_tools.o -lz -lbz2 -o test_splitter

period.o: period.h period.cpp splitter.h
	$(CPP) $(CPPFLAGS) period.cpp -c -o period.o

test_period: period.o test_period.cpp splitter.o bit_tools.o
	$(CPP) $(CPPFLAGS) test_period.cpp period.o splitter.o bit_tools.o -lz -lbz2 -o test_period

repeat_packer: repeat_packer.cpp splitter.o bit_tools.o period.o
	$(CPP) $(CPPFLAGS) repeat_packer.cpp splitter.o bit_tools.o period.o -lz -lbz2 -o repeat_packer

