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
add_definitions(-Wall -Wpointer-arith)
# use c++ 11
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall")


# ======================== easyeai_api =========================
set(toolkit_root ${CMAKE_CURRENT_SOURCE_DIR}/../easyeai-api)
# -I
set(api_inc
   ${toolkit_root}/algorithm_api/person_detect
   ${toolkit_root}/common_api/system_opt
   ${toolkit_root}/common_api/log_manager
   ${toolkit_root}/media_api/frame_queue
   ${toolkit_root}/media_api/endeCode_api
   ${toolkit_root}/netProtocol_api/rtsp
   ${toolkit_root}/peripheral_api/audio
   ${toolkit_root}/peripheral_api/camera
   )
# -L
link_directories(${toolkit_root}/algorithm_api/person_detect)
link_directories(${toolkit_root}/common_api/system_opt)
link_directories(${toolkit_root}/common_api/log_manager)
link_directories(${toolkit_root}/media_api/frame_queue)
link_directories(${toolkit_root}/media_api/endeCode_api)
link_directories(${toolkit_root}/netProtocol_api/rtsp)
link_directories(${toolkit_root}/peripheral_api/audio)
link_directories(${toolkit_root}/peripheral_api/camera)
# -l
LINK_LIBRARIES( system_opt )
LINK_LIBRARIES( log_manager ini )
LINK_LIBRARIES( endeCode_api easymedia rockchip_mpp avcodec swresample avutil)
LINK_LIBRARIES( frame_queue )
LINK_LIBRARIES( rtsp )
LINK_LIBRARIES( camera easymedia rga rkaiq )
LINK_LIBRARIES( audio easymedia asound pthread )
# ======================== easyeai_api =========================


# ========================== custom ============================
# solution project
# -I
set(custom_inc
   ${PROJECT_SOURCE_DIR}/include
#   ${PROJECT_SOURCE_DIR}/include/rga
)
# -L
link_directories(
   ${PROJECT_SOURCE_DIR}/libs
#   ${PROJECT_SOURCE_DIR}/libs/rga
)
# -l
set(custom_libs
   ${OpenCV_LIBS}
   pthread
   crypto
   ssl
)
# source code
aux_source_directory(./src dir_srcs)
aux_source_directory(./src/enCoder dir_srcs)
aux_source_directory(./src/rtspServer dir_srcs)
# ========================== custom ============================

#--------------------------
# execute file
#--------------------------
add_executable(${CURRENT_FOLDER} ${dir_srcs})
target_include_directories(${CURRENT_FOLDER} PRIVATE ${api_inc} ${custom_inc})
target_link_libraries(${CURRENT_FOLDER} ${custom_libs})

