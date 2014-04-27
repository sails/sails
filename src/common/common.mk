INCLUDE=-I../../ -I$(TOPDIR)../../deps/http-parser

CC		= gcc
CXX		= g++
CFLAGS 		= -std=c++11 -g -O0
VPATH		= ${TOPDIR}/base



%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -c -o $@ $<


-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
