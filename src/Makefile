JSON_CPP		= ../deps/json-cpp
PROTOBUF		= ../deps/protobuf
CFLAGS			= -std=c++11 -O0 -g -L../lib -L./
INCLUDE			= -I$(JSON_CPP)/include -I$(PROTOBUF)/src -I./

INSTALL_PATH		= ../dist



OBJECTS=sails.o \
	handle_rpc.o \
	module_load.o service_register.o \
	config.o

sails:$(OBJECTS)
	g++ $(CFLAGS) -Wl,-rpath,../lib/ -o sails $(OBJECTS) -lsails -pthread -lhttp_parser -lprotobuf -ldl -ljson-cpp  -lsails -lprofiler

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<


-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm *.o *.d
install:sails
	if [ ! -d ${INSTALL_PATH} ]; then  mkdir -p ${INSTALL_PATH}; fi
	cp sails ../dist
