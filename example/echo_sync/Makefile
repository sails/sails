SRC=../../src
CFLAGS=-std=c++11 -g
INCLUDE=-I$(SRC)

client: client.o addressbook.pb.o
	g++ $(CFLAGS) client.o addressbook.pb.o -o client -L$(SRC) -lclientrpc -lprotobuf

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<

servicecc=register.cc addressbook.pb.cc
service.so: $(servicecc)
	g++ -shared $(INCLUDE) -fPIC -o service.so $(servicecc) -lprotobuf
