TOPDIR:=../

include ${TOPDIR}/common.mk

TARGET := libcommon_net.a
LOCAL_OBJ= event_loop.o

all : $(TARGET)

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
%.o: %.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<
