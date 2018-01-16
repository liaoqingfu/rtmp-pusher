#3-1 simple-player
#cmake config file
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
project(3-9-resampling_audio)


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


SET(PROJECT_SRC 3-9-resampling_audio.c)
set(PROJECT_LIBS m avfilter avcodec swscale avutil avformat swresample)
add_executable(3-9-resampling_audio ${PROJECT_SRC})
target_link_libraries(3-9-resampling_audio  ${PROJECT_LIBS})