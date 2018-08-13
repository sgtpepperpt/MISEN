list(APPEND CMAKE_MODULE_PATH ${SERVER_DIR})
find_package(TBB REQUIRED)

#set(DEBUG_FLAGS "-g -O0")

set(CMAKE_C_FLAGS "-pthread -O3 -fPIC -Wall -Wno-unused-result")
set(CMAKE_CXX_FLAGS "-pthread -O3 -fPIC -Wall -Wno-unused-result")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

link_directories(${MBEDTLS_DIR}/build/library)

add_executable(Uee ${SERVER_DIR}/StorageServer.cpp)
target_include_directories(Uee PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${SERVER_DIR} ${MBEDTLS_DIR}/include)
target_link_libraries(Uee ${LIB_UNTRUSTED} tbb mbedcrypto mbedtls mbedx509)
