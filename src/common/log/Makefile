TOPDIR:=../

include ${TOPDIR}/common.mk

TARGET := libcommon_log.a
LOCAL_OBJ=time_t.o string.o logging.o

all : $(TARGET)

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
%.o: %.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<
