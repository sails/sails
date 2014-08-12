INCLUDE			= -I../../src/ -I../../deps/http-parser
CFLAGS			= -std=c++11 -fno-omit-frame-pointer -O0 -g -L../../lib

main:main.o
	g++  $(CFLAGS) -Wl,-rpath,../../lib/ -o main main.o -lsails -pthread -lhttp_parser -lprotobuf -ldl -ljson-cpp  -lsails
%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<
