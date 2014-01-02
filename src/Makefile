HTTP_PARSER_PATH = ../deps/http-parser
EV_PATH = ../deps/libev
CFLAGS=-std=c++11 -g
INCLUDE=-I$(EV_PATH) -I$(HTTP_PARSER_PATH)

INSTALL_PATH=../dist

OBJECTS=sails.o http.o connection.o util.o thread_pool.o request.o \
	 response.o filter_default.o handle_default.o \
	handle_proto_decode.o module_load.o service_register.o

sails:$(OBJECTS)
	g++ $(CFLAGS) -Wl,-rpath,../lib/ -o sails $(OBJECTS) -L$(HTTP_PARSER_PATH) -pthread -lhttp_parser -lev -lprotobuf -ldl

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<


-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$



########################client library###################################

clientcc=util.cc request.cc response.cc http.cc client_rpc_channel.cc client_rpc_controller.cc client_rpc.cc
libclient.so:$(clientcc)
	g++ -shared $(CFLAGS) $(INCLUDE) -fPIC -o libclient.so $(clientcc) -lprotobuf -lhttp_parser
	mv libclient.so ../lib/libclient.so

########################client library###################################


clean:
	rm *.o *.d
install:sails
	if [ ! -d ${INSTALL_PATH} ]; then  mkdir -p ${INSTALL_PATH}; fi
	cp sails ../dist
