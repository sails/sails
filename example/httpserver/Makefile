CFLAGS			= -std=c++11 -fno-omit-frame-pointer -W -Wall -O0 -g -L../../ -I./ -I../../../

OBJECTS = server.o

server:$(OBJECTS)
	g++  $(CFLAGS) -Wl,-rpath,../../ -o server $(OBJECTS) -lsails -pthread -ldl -ljsoncpp  -lsails

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<

-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm *.o *.d *.d.*
	-rm server
