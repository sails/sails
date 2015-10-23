CFLAGS			= -std=c++11 -fno-omit-frame-pointer  -W -Wall -O0 -g -L../ -I./ -I../../


OBJECTS = log_test.o connector_test.o netbios_test.o mem_usage_test.o \
	cpu_usage_test.o time_test.o
unit_test : $(OBJECTS)
	g++ $(CFLAGS)  -Wl,-rpath,../ $(OBJECTS) -o unit_test -lsails



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
