SRC=../../src
CFLAGS=-std=c++11 -g
INCLUDE=-I$(SRC)

client: client.o addressbook.pb.o
	g++ $(CFLAGS) client.o addressbook.pb.o -o client -L$(SRC) -lclientrpc -lprotobuf

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<
