SAILS_SRC_PATH = ../src
CFLAGS=-std=c++11 -g
INCLUDE=-I$(SAILS_SRC_PATH)
all:thread_pool_test filter_test response_test

thread_pool_test:thread_pool_test.o ../src/thread_pool.o
	clang++ $(CFLAGS) -o thread_pool_test thread_pool_test.o ../src/thread_pool.o -pthread -lgtest
thread_pool_test.o:thread_pool_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c thread_pool_test.cc

filter_test:filter_test.o
	g++ $(CFLAGS) -o filter_test filter_test.o -lgtest -pthread
filter_test.o:filter_test.cc ../src/filter.h
	g++ $(CFLAGS) $(INCLUDE) -c filter_test.cc

response_test:response_test.o
	g++ $(CFLAGS) -o response response_test.o ../src/response.o -lgtest -pthread
response_test.o:response_test.cc ../src/response.h
	g++ $(CFLAGS) $(INCLUDE) -c response_test.cc
