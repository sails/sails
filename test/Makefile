SAILS_SRC_PATH = ../src
CFLAGS=-I$(SAILS_SRC_PATH) -std=c++11
test:thread_pool.o
	g++ -g -o test thread_pool.o -lgtest -pthread
thread_pool.o:thread_pool.cc
	g++ -g $(CFLAGS) -c thread_pool.cc
