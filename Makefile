# 除了example和test目录
SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "example" && $$9 != "test") print $$9}')


#为了子项目中能用，使用export
export CXX		= g++
export AR		= ar
export RANLIB		= ranlib
#export CXX		= arm-linux-androideabi-g++
#export AR		= arm-linux-androideabi-ar
#export RANLIB		= arm-linux-androideabi-ranlib
#Android 日志库
#export DEPS		= -llog

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
ShareLibSuffix 		= so
ShareLibFlag		= -shared -fPIC -Wl,-soname -Wl,libsails.so
endif
ifeq ($(UNAME), Darwin)
ShareLibSuffix 		= dylib
ShareLibFlag		= -shared -fPIC -install_name @rpath/libsails.dylib
endif
define build_lib
	@for dir in $(SUBDIRS); do make -C $$dir; echo; done
	@for dir in $(SUBDIRS); do cp $$dir/*.a ./; echo "cp "$$dir static library; done 
	rm -f libsails.a
	@for commlib in *.a; do $(AR) x $$commlib; done
	rm -r *.a
	$(AR) cru libsails.a *.o
	$(CXX) ${ShareLibFlag} -o libsails.${ShareLibSuffix} *.o $(DEPS)
	rm -r *.o
	$(RANLIB) libsails.a
endef


all:
	$(build_lib)

INSTALLDIR		= /usr/local
install:
	install -C libsails.a $(INSTALLDIR)/lib/
	#如果是osx系统改变-install_name成和目录一致
	[ "$(UNAME)" = "Darwin" ] &&  { install_name_tool -id $(INSTALLDIR)/lib/libsails.dylib  libsails.dylib;} ||  echo ""
	install -C libsails.${ShareLibSuffix} $(INSTALLDIR)/lib/
	@cd ../; find sails -name '*.h' |xargs tar czf temp.tgz; tar zxvf temp.tgz -C $(INSTALLDIR)/include/; rm temp.tgz; cd sails
	@if [ "$(UNAME)" = "Linux" ]; then ldconfig ;fi

clean:
	@for dir in $(SUBDIRS); do echo $(SUBDIRS); make clean -C $$dir; echo; done
	-rm libsails.a libsails.${ShareLibSuffix}
