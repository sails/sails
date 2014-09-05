CFLAGS			= -std=c++11 -fno-omit-frame-pointer -O0 -g -L../../lib -I./ -I../../src/

OBJECTS = server.o

server:$(OBJECTS)
	g++  $(CFLAGS) -Wl,-rpath,../../lib/ -o server $(OBJECTS) -lsails -pthread -lprotobuf -ldl -ljson-cpp  -lsails

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
