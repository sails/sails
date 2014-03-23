TOPDIR:=../

include ${TOPDIR}/common.mk

TARGET := libcommon_log.a
LOCAL_OBJ= logging.o time_t.o string.o

all : $(TARGET)

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
%.o: %.cc
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<
