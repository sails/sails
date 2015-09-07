TOPDIR:=..

TARGET 		:= libcommon_log.a
LOCAL_OBJ	= logging.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	$(AR) -r $@ $(LOCAL_OBJ)
