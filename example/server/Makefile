CFLAGS			= -std=c++11 -fno-omit-frame-pointer -W -Wall -O0 -g -L../../ -I./ -I../../../

OBJECTS = server.o
CXX = g++


server:$(OBJECTS)
	$(CXX)  $(CFLAGS) -Wl,-rpath,../../ -o server $(OBJECTS) -lsails -pthread -lprofiler

client2:client2.o
	$(CXX)  $(CFLAGS) -Wl,-rpath,../../ -o client2 client2.o -lsails -pthread

client: client.o
	$(CXX) -std=c++11 client.cc -o client -pthread

%.o: %.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<

-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	$(CXX) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm -f *.o *.d *.d.*
	-rm -f server
	-rm -f client
