HTTP_PARSER_PATH = ../deps/http-parser
EV_PATH = ../deps/libev
CFLAGS=-I$(EV_PATH) -I$(HTTP_PARSER_PATH) -std=c++11
sails:sails.o http.o connection.o util.o thread_pool.o request.o \
	 response.o filter_default.o 
	g++ -g -o sails http.o sails.o connection.o request.o \
	filter_default.o response.o util.o thread_pool.o -L$(HTTP_PARSER_PATH) -pthread -lhttp_parser -lev
sails.o:sails.cc sails.h
	g++ -g $(CFLAGS) -c sails.cc
http.o:http.cc http.h
	g++ -g $(CFLAGS) -c http.cc
connection.o:connection.cc connection.h
	g++ -g $(CFLAGS) -c connection.cc
thread_pool.o:thread_pool.cc thread_pool.h
	g++ -g $(CFLAGS) -c thread_pool.cc
request.o:request.h request.cc
	g++ -g $(CFLAGS) -c request.cc
response.h:response.h response.cc
	g++ -g $(CFLAGS) -c response.cc
filter_default.o:filter_default.h filter_default.cc filter.h
	g++ -g $(CFLAGS) -c filter_default.cc
clean:
	rm *.o
