TOPDIR:=../

TARGET := libcommon_net.a
LOCAL_OBJ= event_loop.o http.o connector.o http_connector.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
