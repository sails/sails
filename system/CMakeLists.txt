IF( APPLE )
  set(DIR_SYS_SRCS mem_usage.cc)
ELSE(LINUX)
  set(DIR_SYS_SRCS mem_usage.cc cpu_usage.cc)
endif()
  

ADD_LIBRARY ( system ${DIR_SYS_SRCS})
TARGET_LINK_LIBRARIES(pthread)  