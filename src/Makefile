HTTP_PARSER_PATH = ../deps/http-parser
EV_PATH = ../deps/libev
CFLAGS=-I$(EV_PATH) -I$(HTTP_PARSER_PATH)
sails:sails.o http.o
	g++ -o sails http.o sails.o -L$(HTTP_PARSER_PATH) -lhttp_parser -lev
sails.o:sails.cc sails.h
	g++ $(CFLAGS) -c sails.cc
http.o:http.cc http.h
	g++ $(CFLAGS) -c http.cc

clean:
	rm *.o
