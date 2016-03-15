cmake_minimum_required(VERSION 2.8.8)

project (sails)
include_directories(../)
# 总的设置，让它生成到执行cmake的目录中
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 如果只想针对某个target设置
# set_target_properties(sails_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 但是对于有多个配置的generators（vs, xcode），它会放到每个配置的子目录中(
# 比如vs有debug,release,minsizerel,relwithdebinfo这四种单独配置，
# 如果像上面指定OUTPUT_DIRECTORY，对于每种配置，它会在目录下创建Debug,Release目录，再放target，
# 不过我们可以对相对配置进行指定目录)
#if(XCODE)
#  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_Debug ${CMAKE_BINARY_DIR} )
#  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_Release ${CMAKE_BINARY_DIR} )
#endif()

# 也可以统一指定
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
  string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


  
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -W -Wall -fPIC")
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
  base/time_provider.cc
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

# cmake2.8.8开始支持OBJECT类型，这样动态库，静态库就不用编译两次
# 为了能用于动态库，在要编译选项中加-fPIC
# add_library(objs OBJECT ${SRCLIST})
# add_library(sails_static STATIC $<TARGET_OBJECTS:objs>)
# add_library(sails SHARED $<TARGET_OBJECTS:objs>)
# 但是在xcode中，不能objs.a生成sails.a和sails.dylib，
# 所以这里还是分开编译
# 因为target不能重名
add_library(sails_static STATIC ${SRCLIST})
add_library(sails SHARED ${SRCLIST})

if(ANDROID)
  target_link_libraries(sails log)
  target_link_libraries(sails_static log)
endif()

# 为了防止OUTPUT_NAME设置新的target时清除已经存在的同名target
set_target_properties(sails PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(sails_static PROPERTIES OUTPUT_NAME "sails")
# 设置动态库的版本，api兼容版本
set_target_properties(sails PROPERTIES VERSION 1.0.0 SOVERSION 1)

# LIBRARY指动态库，ARCHIVE指静态库
install(TARGETS sails sails_static
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION lib)
install(DIRECTORY base log crypto net system
  DESTINATION include/sails/
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
  )
