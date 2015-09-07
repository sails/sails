TOPDIR:=..

TARGET := libcommon_net.a
LOCAL_OBJ= http.o connector_list.o connector.o http_server.o http_parser.o \
	mime.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	$(AR) -r $@ $(LOCAL_OBJ)

mime_test:mime_test.o mime.o
	$(CXX) $(CFLAGS) -o mime_test mime_test.o mime.o -pthread -lgtest -L../ -lsails
