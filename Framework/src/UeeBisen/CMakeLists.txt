
#set(DEBUG_FLAGS "-g -O0")

set(CMAKE_C_FLAGS "-pthread -O3 -fPIC -Wall -Wno-unused-result")
set(CMAKE_CXX_FLAGS "-pthread -O3 -fPIC -Wall -Wno-unused-result")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(SPARSEPP_DIR ${CMAKE_SOURCE_DIR}/sparsepp)

link_directories(${MBEDTLS_DIR}/build/library)

add_executable(StorageBisen ${SERVER_BISEN_DIR}/main.cpp ${SERVER_BISEN_DIR}/SseServer.cpp)
target_include_directories(StorageBisen PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${SERVER_BISEN_DIR} ${SPARSEPP_DIR})
target_link_libraries(StorageBisen ${LIB_UNTRUSTED} pthread)
