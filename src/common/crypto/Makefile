TOPDIR:=../

include ${TOPDIR}/common.mk

TARGET := crypto.a
LOCAL_OBJ= MD5Checksum.o md5.o

all : $(TARGET)

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
%.o: %.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<
