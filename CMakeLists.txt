cmake_minimum_required(VERSION 2.8.8)

project (sails)
include_directories(../)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

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
  ${system}
  net/connector.cc
  net/connector_list.cc
  net/http.cc
  net/http_parser.cc
  net/http_server.cc
  net/mime.cc
  net/netbios.cc)

add_library(sails ${SRCLIST})
