HTTP_PARSER_PATH = ../deps/http-parser
EV_PATH = ../deps/libev
CFLAGS=-I$(EV_PATH) -I$(HTTP_PARSER_PATH) -std=c++11
sails:sails.o http.o connection.o util.o thread_pool.o
	g++ -g -o sails http.o sails.o connection.o util.o thread_pool.o -L$(HTTP_PARSER_PATH) -pthread -lhttp_parser -lev
sails.o:sails.cc sails.h
	g++ -g $(CFLAGS) -c sails.cc
http.o:http.cc http.h
	g++ -g $(CFLAGS) -c http.cc
connection.o:connection.cc connection.h
	g++ -g $(CFLAGS) -c connection.cc
thread_pool.o:thread_pool.cc thread_pool.h
	g++ -g $(CFLAGS) -c thread_pool.cc
clean:
	rm *.o
