CFLAGS			= -std=c++11 -fno-omit-frame-pointer -W -Wall -O0 -g -I./

OBJECTS = whois.o

whois:$(OBJECTS)
	g++  $(CFLAGS) -Wl,-rpath,../../ -o whois $(OBJECTS)
%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<

-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm -f *.o *.d *.d.*
	-rm -f server
	-rm -f client
