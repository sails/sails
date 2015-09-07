TOPDIR:=..

TARGET := libcommon_crypto.a
LOCAL_OBJ= MD5Checksum.o md5.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	$(AR) -r $@ $(LOCAL_OBJ)



