cmake_minimum_required(VERSION 2.8.4)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} )
MESSAGE("current project: " ${CURRENT_FOLDER})

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")

# find host dir and set CMAKE_TOOLCHAIN_FILE before call project
execute_process(COMMAND which ${CMAKE_C_COMPILER} 
                OUTPUT_VARIABLE gcc_full_path)
get_filename_component(gcc_full_dir ${gcc_full_path} DIRECTORY)
set(HOST_DIR "${gcc_full_dir}/..")
set(CMAKE_TOOLCHAIN_FILE "${HOST_DIR}/share/buildroot/toolchainfile.cmake")

project(${CURRENT_FOLDER})
add_definitions(-Wall)

# ======================== easyeai_api =========================
set(toolkit_root ${CMAKE_CURRENT_SOURCE_DIR}/../easyeai-api)
# -I
set(api_inc
   ${toolkit_root}/algorithm_api/qrdecode
   ${toolkit_root}/common_api/system_opt
   ${toolkit_root}/common_api/ini_wrapper
   ${toolkit_root}/media_api/frame_queue
   ${toolkit_root}/media_api/endeCode_api
   ${toolkit_root}/netProtocol_api/rtsp
   ${toolkit_root}/peripheral_api/display
   )
# -L
link_directories(${toolkit_root}/algorithm_api/qrdecode)
link_directories(${toolkit_root}/common_api/system_opt)
link_directories(${toolkit_root}/common_api/ini_wrapper)
link_directories(${toolkit_root}/media_api/frame_queue)
link_directories(${toolkit_root}/media_api/endeCode_api)
link_directories(${toolkit_root}/netProtocol_api/rtsp)
link_directories(${toolkit_root}/peripheral_api/display)
# -l
LINK_LIBRARIES( qrdecode zbar )
LINK_LIBRARIES( system_opt )
LINK_LIBRARIES( ini_wrapper ini )
LINK_LIBRARIES( frame_queue endeCode_api rockchip_mpp )
LINK_LIBRARIES( rtsp liveMedia crypto ssl BasicUsageEnvironment UsageEnvironment groupsock )
LINK_LIBRARIES( display drm )
# ======================== easyeai_api =========================


# ========================== custom ============================
# solution project
# -I
set(custom_inc
   ${PROJECT_SOURCE_DIR}/include
   ${PROJECT_SOURCE_DIR}/include/rga
   )
# -L
link_directories(
   ${PROJECT_SOURCE_DIR}/libs 
   ${PROJECT_SOURCE_DIR}/libs/rga
   )
LINK_LIBRARIES( -lrga )
# source code
aux_source_directory(./src dir_srcs)
# ========================== custom ============================

#--------------------------
# execute file
#--------------------------
add_executable(${CURRENT_FOLDER} ${dir_srcs})		#-o
target_include_directories(${CURRENT_FOLDER} PRIVATE ${api_inc} ${custom_inc})	#-I
target_link_libraries(${CURRENT_FOLDER} pthread )	#-l
