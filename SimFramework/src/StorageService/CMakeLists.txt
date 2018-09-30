list(APPEND CMAKE_MODULE_PATH ${STORAGE_DIR})
find_package(TBB REQUIRED)

#set(DEBUG_FLAGS "-g -O0")
set(DEBUG_FLAGS "-O3")

set(CMAKE_C_FLAGS "-pthread ${DEBUG_FLAGS} -fPIC -Wall -Wno-unused-result")
set(CMAKE_CXX_FLAGS "-pthread ${DEBUG_FLAGS} -fPIC -Wall -Wno-unused-result")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

if(SERVER_ONLY) # used for testing, to remove TODO
    set(SPARSEPP_INCLUDE /localssd/gborges/isen-libs/sparsepp)
    set(CASSANDRA_INCLUDE /localssd/gborges/isen-libs/cpp-driver/include)
    set(HIREDIS_INCLUDE /localssd/gborges/isen-libs/hiredis)

    set(LIBUV_LIB_DIR /localssd/gborges/isen-libs/libuv/out/cmake)
    set(CASSANDRA_LIB_DIR /localssd/gborges/isen-libs/cpp-driver/build)
    set(HIREDIS_LIB_DIR /localssd/gborges/isen-libs/hiredis)
endif()

link_directories(${MBEDTLS_DIR}/build/library ${LIBUV_LIB_DIR} ${CASSANDRA_LIB_DIR} ${HIREDIS_LIB_DIR} /usr/local/lib)

add_executable(StorageService ${STORAGE_DIR}/SseServer.cpp ${STORAGE_DIR}/cassie.cpp)
target_include_directories(StorageService PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${STORAGE_DIR} ${SPARSEPP_INCLUDE} ${CASSANDRA_INCLUDE} ${HIREDIS_INCLUDE} /usr/local/include)
target_link_libraries(StorageService ${LIB_UNTRUSTED} pthread cassandra hiredis tbb)
