#3-1 simple-player
#cmake config file
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
project(3-3-decode_video)


#使用C++编译器
SET(CMAKE_C_COMPILER g++)			
SET(CMAK_CXX_COMPILER g++)
#支持C++ 11
add_compile_options(-std=c++11)
add_compile_options(-w)

# Add header file include directories
#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
#INCLUDE_DIRECTORIES(/usr/local/include/)
#LINK_DIRECTORIES(/usr/local/lib/)


SET(PROJECT_SRC 3-3-decode_video.c)
set(PROJECT_LIBS avcodec swscale avutil avformat swresample)
add_executable(3-3-decode_video ${PROJECT_SRC})
target_link_libraries(3-3-decode_video  ${PROJECT_LIBS})