cmake_minimum_required(VERSION 3.10)

set(BINARY_NAME elastic_irc_bot)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
set(LIBRARIES "")

project(${BINARY_NAME} C)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-DDEBUG=1)
endif()

find_path(PTHREADS_INCLUDE_DIR NAMES threads pthread pthreads)
find_library(PTHREADS_LIBRARY NAMES threads pthread pthreads REQUIRED)
if (PTHREADS_LIBRARY)
	include_directories(${PTHREADS_INCLUDE_DIR})
	set(LIBRARIES ${LIBRARIES} ${PTHREADS_LIBRARY})
else(PTHREADS_LIBRARY)
	message(FATAL_ERROR "Unable to locate pthreads")
endif(PTHREADS_LIBRARY)


find_path(CURL_INCLUDE_DIR NAMES libcurl curl)
find_library(CURL_LIBRARY NAMES libcurl curl REQUIRED)
if (CURL_LIBRARY)
	include_directories(${CURL_INCLUDE_DIR})
	set(LIBRARIES ${LIBRARIES} ${CURL_LIBRARY})
else(CURL_LIBRARY)
	message(FATAL_ERROR "Unable to locate curl")
endif(CURL_LIBRARY)

include_directories("lib/sts-queue/src")

add_executable(${BINARY_NAME}
		main.c
		lib/sts-queue/src/sts_queue/sts_queue.c
		irc_token.c
		irc_lexer.c
		irc_message_parser.c
		message_bus.c
		elastic_search.c)

target_link_libraries(elastic_irc_bot ${LIBRARIES})