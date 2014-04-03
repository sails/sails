SAILS_SRC_PATH = ../src
CFLAGS=-std=c++11 -g -L../lib
INCLUDE=-I$(SAILS_SRC_PATH)
all:thread_pool_test filter_test response_test


util_test:util_test.o
	g++ $(CFLAGS) -o util_test util_test.o ../src/util.o -lgtest -pthread
util_test.o:util_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c util_test.cc
thread_pool_test:thread_pool_test.o ../src/thread_pool.o
	clang++ $(CFLAGS) -o thread_pool_test thread_pool_test.o ../src/thread_pool.o -pthread -lgtest
thread_pool_test.o:thread_pool_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c thread_pool_test.cc

filter_test:filter_test.o
	g++ $(CFLAGS) -o filter_test filter_test.o -lgtest -pthread
filter_test.o:filter_test.cc ../src/filter.h
	g++ $(CFLAGS) $(INCLUDE) -c filter_test.cc

response_test:response_test.o
	g++ $(CFLAGS) -o response_test response_test.o ../src/response.o -lgtest -pthread
response_test.o:response_test.cc ../src/response.h
	g++ $(CFLAGS) $(INCLUDE) -c response_test.cc
config_test:config_test.o
	g++ $(CFLAGS) $(INCLUDE) -o config_test config_test.o ../src/config.o -ljson -lgtest -pthread
config_test.o:config_test.cc ../src/config.h
	g++ $(CFLAGS) $(INCLUDE) -c config_test.cc
time_test:time_test.o
	g++ $(CFLAGS) $(INCLUDE) -o time_test time_test.o -lcommon_system -lgtest -pthread
time_test.o:time_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c time_test.cc

logging_test:logging_test.o
	g++ $(CFLAGS) $(INCLUDE) -o logging_test logging_test.o -lcommon_log -lgtest -pthread
logging_test.o:logging_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c logging_test.cc

event_loop_test:event_loop_test.o
	g++ $(CFLAGS) $(INCLUDE) -o event_loop_test event_loop_test.o -lcommon_net -lgtest -pthread
event_loop_test.o:event_loop_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c event_loop_test.cc
