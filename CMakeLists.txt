set(PROJECT_NAME zia)

cmake_minimum_required(VERSION 3.10)
project(${PROJECT_NAME})


set(CMAKE_CXX_STANDARD 17)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

else ()
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -W -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
endif ()


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(zany/lib/)
include_directories(srcs/)
include_directories(includes/)

set(INCLUDES
	includes/Zia.hpp
)

set(SRCS
	zany/lib/Zany.hpp
	srcs/main.cpp
	srcs/Zia.cpp
)

add_executable(${PROJECT_NAME} ${INCLUDES} ${SRCS})

find_package (Threads)
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
