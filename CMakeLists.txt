set(PROJECT_NAME zia)
cmake_minimum_required(VERSION 3.10)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")

else ()
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb")
	set(CMAKE_CXX_FLAGS_DEBUG
		"${CMAKE_CXX_FLAGS_DEBUG} -ggdb -W -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter"
	)
endif ()


find_package (Threads)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(zany/lib/)
include_directories(srcs/)
include_directories(includes/)

set(ZIA_INCLUDES
	includes/Zia.hpp
)

set(ZIA_SRCS
	zany/lib/Zany.hpp
	srcs/main.cpp
	srcs/Zia.cpp
)

add_executable(${PROJECT_NAME} ${ZIA_INCLUDES} ${ZIA_SRCS})

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

set(JSONPARSER_NAME JsonParserModule)
set(HTTP_NAME HttpModule)
set(SSLCORE_NAME SslCoreModule)
set(PHPCGI_NAME PhpCgiModule)
set(MANAGER_NAME ManagerModule)

set(JSONPARSER_MODULE_SRCS
	zany/lib/Zany.hpp
	srcs/modules/JsonParser/Module.cpp
	srcs/modules/JsonParser/Parser.hpp
	srcs/modules/JsonParser/Parser.cpp
)

set(HTTP_MODULE_SRCS
	zany/lib/Zany.hpp
	srcs/modules/Http/Module.cpp
)

set(PHPCGI_MODULE_SRCS
		zany/lib/Zany.hpp
		srcs/modules/PhpCgi/Module.cpp
)

set(MANAGER_MODULE_SRCS
		zany/lib/Zany.hpp
		srcs/modules/Manager/Module.cpp
		srcs/modules/Manager/TcpConnexion.cpp srcs/modules/Manager/TcpConnexion.hpp)

set(SSLCORE_MODULE_SRCS
	zany/lib/Zany.hpp
	srcs/modules/SslCore/Module.cpp
	srcs/modules/SslCore/Listener.hpp
)

add_library(${JSONPARSER_NAME} SHARED ${JSONPARSER_MODULE_SRCS})
add_library(${HTTP_NAME} SHARED ${HTTP_MODULE_SRCS})
add_library(${SSLCORE_NAME} SHARED ${SSLCORE_MODULE_SRCS})
add_library(${PHPCGI_NAME} SHARED ${PHPCGI_MODULE_SRCS})
add_library(${MANAGER_NAME} SHARED ${MANAGER_MODULE_SRCS})
target_link_libraries(${SSLCORE_NAME} ${CONAN_LIBS})
target_link_libraries(${MANAGER_NAME} ${CONAN_LIBS})
target_link_libraries(${HTTP_NAME} ${CONAN_LIBS})
target_link_libraries(${PHPCGI_NAME} ${CONAN_LIBS})