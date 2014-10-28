TOPDIR:=..

TARGET := libcommon_sys.a
LOCAL_OBJ= cpu_usage.o mem_usage.o

all : $(TARGET)

include ${TOPDIR}/common.mk

$(TARGET):$(LOCAL_OBJ)
	ar -r $@ $(LOCAL_OBJ)

cpu_usage_test:cpu_usage_test.o cpu_usage.o
	$(CXX) $(CFLAGS) -o cpu_usage_test cpu_usage_test.o cpu_usage.o -pthread -lgtest -L../ -lsails
mem_usage_test:mem_usage_test.o mem_usage.o
	$(CXX) $(CFLAGS) -o mem_usage_test mem_usage_test.o mem_usage.o -pthread -lgtest -L../ -lsails
