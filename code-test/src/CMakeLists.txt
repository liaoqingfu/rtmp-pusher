#rtmp player
#cmake config file
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#设置子目录
set(SUB_DIR_LIST audio_code demo)

#使用C++编译器
SET(CMAKE_C_COMPILER g++)			
SET(CMAK_CXX_COMPILER g++)
#支持C++ 11
add_compile_options(-std=c++11)
#设置需要链接的库
set(LINK_LIB_LIST)

# Add header file include directories
#include_directories(src/third_lib/libFaad)
set(INSTALL_PATH_LIB lib)
set(INSTALL_PATH_INCLUDE include)
foreach(SUB_DIR ${SUB_DIR_LIST})
    #遍历源文件
    aux_source_directory(${SUB_DIR} SRC_LIST)
	include_directories(${SUB_DIR})
	MESSAGE(STATUS "SUB_DIR:" ${SUB_DIR})
	MESSAGE(STATUS "SRC_LIST result: " ${SRC_LIST})
endforeach(SUB_DIR ${SUB_DIR_LIST})

#使用boost
SET(Boost_USE_STATIC_LIBS        ON) # only find static libs
SET(Boost_USE_MULTITHREADED      ON)

SET(Boost_LIBRARIES boost_system boost_date_time)

set(PROJECT_LIBS  m fdk-aac mad)
add_executable(code-test ${SRC_LIST})
target_link_libraries(code-test ${PROJECT_LIBS})