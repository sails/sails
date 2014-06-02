HTTP_PARSER 		= ../deps/http-parser
JSON_CPP		= ../deps/json-cpp
PROTOBUF		= ../deps/protobuf
CFLAGS			= -std=c++11 -g -fno-omit-frame-pointer -O0 -L../lib
INCLUDE			= -I$(HTTP_PARSER) -I$(JSON_CPP)/include -I$(PROTOBUF)/src -I./

INSTALL_PATH		= ../dist



OBJECTS=sails.o connection.o \
	filter_default.o handle_default.o \
	handle_proto_decode.o module_load.o service_register.o \
	config.o

sails:$(OBJECTS)
	g++ $(CFLAGS) -Wl,-rpath,../lib/ -o sails $(OBJECTS) -lsails -pthread -lhttp_parser -lprotobuf -ldl -ljson-cpp  -lsails

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<


-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$



########################client library###################################

clientcc=client_rpc_channel.cc client_rpc_controller.cc client_rpc.cc
libclient.so:$(clientcc)
	g++ -shared $(CFLAGS) $(INCLUDE) -fPIC -o libclient.so $(clientcc) -Wl,--whole-archive  -lsails -Wl,--no-whole-archive  -lprotobuf -lhttp_parser
	mv libclient.so ../lib/libclient.so

########################client library###################################


clean:
	rm *.o *.d
install:sails
	if [ ! -d ${INSTALL_PATH} ]; then  mkdir -p ${INSTALL_PATH}; fi
	cp sails ../dist
