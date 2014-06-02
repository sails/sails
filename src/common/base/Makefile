TOPDIR:=../

TARGET := libcommon_base.a
LOCAL_OBJ= filesys.o string.o time_t.o util.o thread_pool.o event_loop.o timer.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
