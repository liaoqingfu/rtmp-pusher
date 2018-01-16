#3-1 simple-player
#cmake config file
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
project(yuv-encoder)


#使用C++编译器
SET(CMAKE_C_COMPILER g++)			
SET(CMAK_CXX_COMPILER g++)
#支持C++ 11
add_compile_options(-std=c++11)
add_compile_options(-w)

# Add header file include directories
#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(/usr/local/ffmpeg/include/)
LINK_DIRECTORIES(/usr/local/ffmpeg/lib/)


SET(PROJECT_SRC 3-2-main.c)
set(PROJECT_LIBS x264 avcodec swscale avutil avformat swresample SDL2)
add_executable(yuv-encoder ${PROJECT_SRC})
target_link_libraries(yuv-encoder  ${PROJECT_LIBS})
