DIRS	= base crypto log net system

include ./define.mk

all:
	@for dir in $(DIRS); do make -C $$dir; echo; done
	@for dir in $(DIRS); do cp $$dir/*.a ./; echo "cp "$$dir static library; done 
	rm -f libsails.a
	@for commlib in *.a; do $(AR) x $$commlib; done
	rm -r *.a
	$(AR) cru libsails.a *.o
	$(CXX) -shared -fPIC -o libsails.so *.o
	rm -r *.o
	$(RANLIB) libsails.a


clean:
	@for dir in $(DIRS); do make clean -C $$dir; echo; done
	-rm libsails.a libsails.so
