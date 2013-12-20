HTTP_PARSER_PATH = ../deps/http-parser
EV_PATH = ../deps/libev
CFLAGS=-std=c++11 -g
INCLUDE=-I$(EV_PATH) -I$(HTTP_PARSER_PATH)
OBJECTS=sails.o http.o connection.o util.o thread_pool.o request.o \
	 response.o filter_default.o handle_default.o

sails:$(OBJECTS)
	g++ $(CFLAGS) -o sails $(OBJECTS) -L$(HTTP_PARSER_PATH) -pthread -lhttp_parser -lev

%.o: %.cc %.h
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<
#sails.o:sails.cc sails.h
#	g++ $(CFLAGS) $(INCLUDE) -c sails.cc
#http.o:http.cc http.h
#	g++ $(CFLAGS) $(INCLUDE) -c http.cc
#connection.o:connection.cc connection.h
#	g++ $(CFLAGS) $(INCLUDE) -c connection.cc
#thread_pool.o:thread_pool.cc thread_pool.h
#	g++ $(CFLAGS) $(INCLUDE) -c thread_pool.cc
#request.o:request.h request.cc
#	g++ $(CFLAGS) $(INCLUDE) -c request.cc
#response.h:response.h response.cc
#	g++ $(CFLAGS) $(INCLUDE) -c response.cc
#filter_default.o:filter_default.h filter_default.cc filter.h
#	g++ $(CFLAGS) $(INCLUDE) -c filter_default.cc
#handle_default.o:handle_default.h handle_default.cc handle.h
#	g++ $(CFLAGS) $(INCLUDE) -c handle_default.cc
clean:
	rm *.o
install:sails
	cp sails ../dist
