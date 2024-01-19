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
# use c++ 11
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall")


# ======================== easyeai_api =========================
set(toolkit_root ${CMAKE_CURRENT_SOURCE_DIR}/../easyeai-api)
# -I
set(api_inc
   ${toolkit_root}/algorithm_api/person_detect
   ${toolkit_root}/common_api/system_opt
   ${toolkit_root}/peripheral_api/camera
   ${toolkit_root}/peripheral_api/display
   )
# -L
link_directories(${toolkit_root}/algorithm_api/person_detect)
link_directories(${toolkit_root}/common_api/system_opt)
link_directories(${toolkit_root}/peripheral_api/camera)
link_directories(${toolkit_root}/peripheral_api/display)
# -l
LINK_LIBRARIES( person_detect rknn_api )
LINK_LIBRARIES( system_opt pthread )
LINK_LIBRARIES( display easymedia rga )
LINK_LIBRARIES( camera easymedia rga rkaiq)
# ======================== easyeai_api =========================


# ========================== custom ============================
# solution project
find_package(OpenCV REQUIRED)
# -I
set(custom_inc
   ${PROJECT_SOURCE_DIR}/include
   ${OpenCV_INCLUDE_DIRS}
)
# -L
link_directories(${PROJECT_SOURCE_DIR}/libs)
# -l
set(custom_libs
   ${OpenCV_LIBS}
)
# source code
aux_source_directory(./src dir_srcs)
# ========================== custom ============================

#--------------------------
# execute file
#--------------------------
add_executable(${CURRENT_FOLDER} ${dir_srcs})
target_include_directories(${CURRENT_FOLDER} PRIVATE ${api_inc} ${custom_inc})
target_link_libraries(${CURRENT_FOLDER} ${custom_libs})

