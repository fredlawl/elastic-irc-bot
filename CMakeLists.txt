cmake_minimum_required(VERSION 3.10)

set(BINARY_NAME elastic_irc_bot)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wshadow -Wextra -Wvla -Wmissing-field-initializers")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
set(LIBRARIES "")
set(TEST_LIBRARIES "")

project(${BINARY_NAME} C)

link_directories(/usr/local/lib)

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


find_path(CRITERION_INCLUDE_DIR NAMES criterion)
find_library(CRITERION_LIBRARY NAMES criterion REQUIRED)
if (CRITERION_LIBRARY)
	include_directories(${CRITERION_INCLUDE_DIR})
	set(TEST_LIBRARIES ${TEST_LIBRARIES} ${CRITERION_LIBRARY})
else(CRITERION_LIBRARY)
	message(FATAL_ERROR "Unable to locate criterion")
endif(CRITERION_LIBRARY)


include_directories("lib/sts-queue/src")

add_library(libelasticirc STATIC
		lib/sts-queue/src/sts_queue/sts_queue.c
		irc_token.c
		irc_lexer.c
		irc_message_parser.c
		message_bus.c
		elastic_search.c)

add_executable(${BINARY_NAME} main.c)
target_link_libraries(elastic_irc_bot ${LIBRARIES} libelasticirc)

add_executable(${BINARY_NAME}_test test/main.c)
target_link_libraries(${BINARY_NAME}_test ${TEST_LIBRARIES} libelasticirc)