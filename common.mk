INCLUDE		= -I$(TOPDIR)/../

include $(TOPDIR)/define.mk

CFLAGS 		= -std=c++11 -O0 -g -L$(TOPDIR)/base -W -Wall
VPATH		= ${TOPDIR}/base
DEPSLIBS 	= common_base


%.o: %.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -fPIC -c -o $@ $<


-include $(LOCAL_OBJ:.o=.d) # $(OBJECTS.o=.d)replace all *.o to *.d

%.d: %.cc
	set -e; rm -f $@; \
	$(CXX) -MM $(CFLAGS) $(INCLUDE) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm -f *.o *.d *.a  *.d.*
