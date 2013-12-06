SAILS_SRC_PATH = ../src
CFLAGS=-I$(SAILS_SRC_PATH) -std=c++11
all:thread_pool_test

thread_pool_test:thread_pool_test.o ../src/thread_pool.o
	clang++ -g -o thread_pool_test thread_pool_test.o ../src/thread_pool.o -pthread -lgtest
thread_pool_test.o:thread_pool_test.cc
	g++ -g $(CFLAGS) -c thread_pool_test.cc
