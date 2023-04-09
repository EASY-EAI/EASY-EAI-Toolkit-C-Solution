cmake_minimum_required(VERSION 2.8.4)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} )
MESSAGE("current project: " ${CURRENT_FOLDER})

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CROSSCOMPILING TRUE)

cmake_host_system_information(RESULT arch_value QUERY OS_PLATFORM)
message(STATUS "architecture: " ${arch_value})

if(NOT "${arch_value}" STREQUAL "armv7l")
	include ($ENV{HOME}/configs/cross.cmake)
endif()

#若要使用${CMAKE_SYSROOT}变量，project必须要在set(CMAKE_TOOLCHAIN_FILE "${HOST_DIR}/share/buildroot/toolchainfile.cmake")后面设置
project(${CURRENT_FOLDER})
add_definitions(-Wall -Wpointer-arith)
# use c++ 11
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall")


# ======================== easyeai_api =========================
set(toolkit_root ${CMAKE_CURRENT_SOURCE_DIR}/../easyeai-api)
# -I
set(api_inc
   ${toolkit_root}/common_api/system_opt
   ${toolkit_root}/common_api/ini_wrapper
   ${toolkit_root}/common_api/log_manager
   ${toolkit_root}/media_api/frame_queue
   ${toolkit_root}/media_api/endeCode_api
   ${toolkit_root}/netProtocol_api/rtsp
   ${toolkit_root}/peripheral_api/display
   ${toolkit_root}/peripheral_api/network
   )
# -L
link_directories(${toolkit_root}/common_api/system_opt)
link_directories(${toolkit_root}/common_api/ini_wrapper)
link_directories(${toolkit_root}/common_api/log_manager)
link_directories(${toolkit_root}/media_api/frame_queue)
link_directories(${toolkit_root}/media_api/endeCode_api)
link_directories(${toolkit_root}/netProtocol_api/rtsp)
link_directories(${toolkit_root}/peripheral_api/display)
link_directories(${toolkit_root}/peripheral_api/network)
# -l
LINK_LIBRARIES( system_opt pthread )
LINK_LIBRARIES( ini_wrapper ini )
LINK_LIBRARIES( log_manager )
LINK_LIBRARIES( frame_queue log_manager )
LINK_LIBRARIES( endeCode_api easymedia rockchip_mpp )
LINK_LIBRARIES( rtsp )
LINK_LIBRARIES( network yaml-cpp )
LINK_LIBRARIES( display easymedia rga )
# ======================== easyeai_api =========================


# ========================== custom ============================
# solution project
find_package(OpenCV REQUIRED)
# -I
set(custom_inc
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/rga
    ${OpenCV_INCLUDE_DIRS}
)
# -L
link_directories(
   ${PROJECT_SOURCE_DIR}/libs 
   ${PROJECT_SOURCE_DIR}/libs/rga
)
# -l
LINK_LIBRARIES(  
    opencv_calib3d 
    opencv_core 
    opencv_dnn 
    opencv_features2d 
    opencv_flann 
    opencv_highgui 
    opencv_imgcodecs 
    opencv_imgproc 
    opencv_ml 
    opencv_objdetect 
    opencv_photo 
    opencv_shape 
    opencv_stitching 
    opencv_superres 
    opencv_videoio 
    opencv_video 
    opencv_videostab
)
# source code
aux_source_directory(./src dir_srcs)
aux_source_directory(./src/capturer dir_srcs)
aux_source_directory(./src/player dir_srcs)
# ========================== custom ============================

#--------------------------
# execute file
#--------------------------
add_executable(${CURRENT_FOLDER} ${dir_srcs})		#-o
target_include_directories(${CURRENT_FOLDER} PRIVATE ${api_inc} ${custom_inc})

