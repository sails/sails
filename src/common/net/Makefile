TOPDIR:=../

TARGET := libcommon_net.a
LOCAL_OBJ= http.o connector_list.o connector.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)
