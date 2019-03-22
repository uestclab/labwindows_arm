cmake_minimum_required(VERSION 3.5)
option(PUBLIC "This is a default option for PUBLIC" ON)
message(STATUS "build public platform switch: "${PUBLIC})
project(devAdapter)

if(${PUBLIC} STREQUAL "ON")
	message(STATUS "start to build ARM platform.")
	# cross complie
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(CMAKE_SYSTEM_PROCESSOR arm) 
	SET(TOOLCHAIN_DIR "/home/gyl/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi")
	SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR})
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

	SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-gcc-6.2.1)
	SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-g++)
	link_directories("${PROJECT_SOURCE_DIR}/dependence/lib_arm")
	# cross complie
	# zlog
	link_directories("/home/gyl/liqingSpace/code/GW_arm_server/temp/lib/")
	include_directories("/home/gyl/liqingSpace/code/GW_arm_server/temp/include/")

elseif(${PUBLIC} STREQUAL "OFF")
    message(STATUS "start to build PC platform.")
	add_definitions("-D USE_STUB")
	link_directories("${PROJECT_SOURCE_DIR}/dependence/lib")
endif(${PUBLIC} STREQUAL "ON")

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")


SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")  

include_directories("${PROJECT_SOURCE_DIR}/dependence")
include_directories("${PROJECT_SOURCE_DIR}/dependence/include")


if(${PUBLIC} STREQUAL "ON") # cross complie
	SET(SOURCE_FILES devAdapter.c process.c csiLoopMain.c utility.c procBroker.c exception.c)
elseif(${PUBLIC} STREQUAL "OFF") # stub test on pc
	SET(SOURCE_FILES devAdapter.c process.c stub.c utility.c)
endif(${PUBLIC} STREQUAL "ON")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} cjson)
target_link_libraries(${PROJECT_NAME} -pthread)
target_link_libraries(${PROJECT_NAME} -lzlog)
if(${PUBLIC} STREQUAL "ON")
	target_link_libraries(${PROJECT_NAME} cst)
	target_link_libraries(${PROJECT_NAME} broker)
	target_link_libraries(${PROJECT_NAME} mosquitto)
endif(${PUBLIC} STREQUAL "ON")














