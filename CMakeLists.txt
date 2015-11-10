cmake_minimum_required(VERSION 2.8.8)

project (sails)
include_directories(../)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# osx会设置动态库的install_name为@rpath/libxxx.dylib
set(CMAKE_MACOSX_RPATH 1)

# 当编译android时，cmake ../ -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake

# 检测平台
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(Darwin ON)
endif()

if(LINUX AND NOT ANDROID)
  set(system
    system/cpu_usage.cc
    system/mem_usage.cc)
elseif(Darwin OR ANDROID)
  set(system system/mem_usage.cc)
endif()

set(SRCLIST
  base/event_loop.cc
  base/filesys.cc
  base/memory_pool.cc
  base/string.cc
  base/thread_pool.cc
  base/time_t.cc
  base/timer.cc
  base/util.cc
  log/logging.cc
  crypto/MD5Checksum.cc
  crypto/hash.cc
  crypto/md5.cc
  ${system}
  net/connector.cc
  net/connector_list.cc
  net/http.cc
  net/http_parser.cc
  net/http_server.cc
  net/mime.cc
  net/netbios.cc)

#因为target不能重名
add_library(sails_static STATIC ${SRCLIST})
add_library(sails SHARED ${SRCLIST})

# 为了防止OUTPUT_NAME设置新的target时清除已经存在的同名target
set_target_properties(sails PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(sails_static PROPERTIES OUTPUT_NAME "sails")
# 设置动态库的版本，api兼容版本
set_target_properties(sails PROPERTIES VERSION 1.0.0 SOVERSION 1)

#LIBRARY指动态库，ARCHIVE指静态库
install(TARGETS sails sails_static
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION lib)
install(DIRECTORY base log crypto net system
   DESTINATION include/sails/
   FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
