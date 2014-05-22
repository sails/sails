INCLUDE=-I../../ -I$(TOPDIR)../../deps/http-parser

CC		= gcc
CXX		= g++
CFLAGS 		= -std=c++11 -g -O0 -L$(TOPDIR)base -pthread
VPATH		= ${TOPDIR}/base
DEPSLIBS 	= common_base


%.o: %.cc
	g++ $(CFLAGS) $(INCLUDE) -fPIC -c -o $@ $< -l$(DEPSLIBS)


-include $(OBJECTS:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	g++ -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
