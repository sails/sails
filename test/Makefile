SAILS_SRC_PATH = ../src
CFLAGS=-I$(SAILS_SRC_PATH) -std=c++11
all:thread_pool_test filter_test

thread_pool_test:thread_pool_test.o ../src/thread_pool.o
	clang++ -g -o thread_pool_test thread_pool_test.o ../src/thread_pool.o -pthread -lgtest
thread_pool_test.o:thread_pool_test.cc
	g++ -g $(CFLAGS) -c thread_pool_test.cc

filter_test:filter_test.o
	g++ -g -o filter_test filter_test.o -lgtest -pthread
filter_test.o:filter_test.cc ../src/filter.h
	g++ -g $(CFLAGS) -c filter_test.cc
