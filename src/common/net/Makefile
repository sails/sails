TOPDIR:=../

TARGET := libcommon_net.a
LOCAL_OBJ= http.o http_connector.o com_connector.o connector.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
