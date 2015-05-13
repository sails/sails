CFLAGS			= -std=c++11 -fno-omit-frame-pointer  -W -Wall -O0 -g -gstabs+ -L../../ -I./ -I../../../

OBJECTS = server.o game_world.o game_room.o player.o product.o config.o

server:$(OBJECTS)
	g++  $(CFLAGS) -Wl,-rpath,../../ -o server $(OBJECTS) -lsails -pthread -lprotobuf -ldl -ljsoncpp  -lsails  -lcurl

%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<

-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm *.o *.d
	-rm *.d.*
	-rm server
