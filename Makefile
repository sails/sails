# 除了example和test目录
SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "example" && $$9 != "test") print $$9}')


#为了项目中能用，使用export
export CXX		= g++
export AR		= ar
export RANLIB		= ranlib
#export CXX		= arm-linux-androideabi-g++
#export AR		= arm-linux-androideabi-ar
#export RANLIB		= arm-linux-androideabi-ranlib
#Android 日志库
#export DEPS		= -llog

define build_lib
	@for dir in $(SUBDIRS); do make -C $$dir; echo; done
	@for dir in $(SUBDIRS); do cp $$dir/*.a ./; echo "cp "$$dir static library; done 
	rm -f libsails.a
	@for commlib in *.a; do $(AR) x $$commlib; done
	rm -r *.a
	$(AR) cru libsails.a *.o
	$(CXX) -shared -fPIC -o libsails.so *.o $(DEPS)
	rm -r *.o
	$(RANLIB) libsails.a
endef


all:
	$(build_lib)

clean:
	@for dir in $(SUBDIRS); do make clean -C $$dir; echo; done
	-rm libsails.a libsails.so
