SAILS_SRC_PATH = ../src
CFLAGS=-std=c++11 -g -L../lib
INCLUDE=-I$(SAILS_SRC_PATH) -I../deps/json-cpp/include
all:thread_pool_test filter_test response_test
CXX = g++

thread_pool_test:thread_pool_test.o
	clang++ $(CFLAGS) -o thread_pool_test thread_pool_test.o -pthread -lsails -lgtest
thread_pool_test.o:thread_pool_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c thread_pool_test.cc

filter_test:filter_test.o
	g++ $(CFLAGS) -o filter_test filter_test.o -lgtest -pthread
filter_test.o:filter_test.cc ../src/filter.h
	g++ $(CFLAGS) $(INCLUDE) -c filter_test.cc

response_test:response_test.o
	g++ $(CFLAGS) -o response_test response_test.o -lsails -lgtest -pthread
response_test.o:response_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c response_test.cc
config_test:config_test.o
	g++ $(CFLAGS) $(INCLUDE) -o config_test config_test.o ../src/config.o -Wl,-rpath,../lib/ -ljson-cpp -lgtest -pthread
config_test.o:config_test.cc ../src/config.h
	g++ $(CFLAGS) $(INCLUDE) -c config_test.cc
time_test:time_test.o
	g++ $(CFLAGS) $(INCLUDE) -o time_test time_test.o -lsails -lgtest -pthread
time_test.o:time_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c time_test.cc

logging_test:logging_test.o
	$(CXX) $(CFLAGS) $(INCLUDE) -o logging_test logging_test.o -lsails -lgtest -pthread
logging_test.o:logging_test.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -c logging_test.cc

event_loop_test:event_loop_test.o
	g++ $(CFLAGS) $(INCLUDE) -o event_loop_test event_loop_test.o -lsails -lgtest -pthread
event_loop_test.o:event_loop_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c event_loop_test.cc

timer_test:timer_test.o
	g++ $(CFLAGS) $(INCLUDE) -o timer_test timer_test.o -lsails -lgtest -pthread
timer_test.o:timer_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c timer_test.cc
shared_pointer_test:shared_pointer_test.o
	g++ $(CFLAGS) $(INCLUDE) -o shared_pointer_test shared_pointer_test.o -lgtest -pthread
shared_pointer_test.o:shared_pointer_test.cc
	g++ $(CFLAGS) $(INCLUDE) -c shared_pointer_test.cc
