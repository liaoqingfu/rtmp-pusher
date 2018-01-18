#3-1 simple-player
#cmake config file
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
project(3-5-encoder_yuv_to_h264)


#使用C++编译器
SET(CMAKE_C_COMPILER gcc)			
SET(CMAK_CXX_COMPILER gcc)
#支持C++ 11
#add_compile_options(-std=c++11)
add_compile_options(-w)

# Add header file include directories
#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
#INCLUDE_DIRECTORIES(/usr/local/include/)
#LINK_DIRECTORIES(/usr/local/lib/)


SET(PROJECT_SRC 3-5-encoder_yuv_to_h264.c)
set(PROJECT_LIBS  avcodec swscale avutil avformat swresample x264)
add_executable(3-5-encoder_yuv_to_h264 ${PROJECT_SRC})
target_link_libraries(3-5-encoder_yuv_to_h264  ${PROJECT_LIBS})