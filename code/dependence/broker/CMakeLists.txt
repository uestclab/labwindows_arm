cmake_minimum_required(VERSION 3.5)

option(PUBLIC "This is a default option for PUBLIC" OFF)
message(STATUS "build public platform switch: "${PUBLIC})
project(Mylibrary)


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
	# cross complie
elseif(${PUBLIC} STREQUAL "OFF")
    message(STATUS "start to build PC platform.")
endif(${PUBLIC} STREQUAL "ON")

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/..")


SET(LIB_NAME broker)
SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")  

SET(ALL_LIBRARY_SRCS broker.c)

add_library(${LIB_NAME} SHARED ${ALL_LIBRARY_SRCS})

target_link_libraries(${LIB_NAME} -lpthread)
target_link_libraries(${LIB_NAME} -lmosquitto)


if(${PUBLIC} STREQUAL "ON")
    message(STATUS "start to install ARM platform library.")
	install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib_arm/)
elseif(${PUBLIC} STREQUAL "OFF")
    message(STATUS "start to build PC platform library.")
	install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib/)
endif(${PUBLIC} STREQUAL "ON")

install(FILES ${PROJECT_SOURCE_DIR}/broker.h ${PROJECT_SOURCE_DIR}/common.h ${PROJECT_SOURCE_DIR}/list.h DESTINATION include/)

