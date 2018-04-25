# sgx vars
if (NOT DEFINED ENV{SGX_SDK})
    set(SGX_SDK "/opt/intel/sgxsdk")
else()
    set(SGX_SDK $ENV{SGX_SDK})
endif ()

# include paths for the sgx versions of standard libs
set(SGX_INCLUDE_PATHS ${SGX_SDK}/include ${SGX_SDK}/include/libcxx ${SGX_SDK}/include/tlibc)

# flags
set(DEBUG_FLAGS "-g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_FLAGS} -c -Wall -fPIC -Wall -m64 -fvisibility=hidden -fpie -fstack-protector -nostdinc++")
set(CMAKE_CXX_STANDARD 11)

# define target library
set(KMEANS_SRCS ${EXTERN_DIR}/src/seq-kmeans/util.cpp ${EXTERN_DIR}/src/seq-kmeans/kmeans.cpp ${EXTERN_DIR}/src/seq-kmeans/bagofwords.cpp)
set(UTIL_SRCS ${EXTERN_DIR}/src/parallel.cpp ${EXTERN_DIR}/src/scoring.cpp ${EXTERN_DIR}/src/img_processing.cpp ${EXTERN_DIR}/src/util.cpp)
set(SOURCES ${EXTERN_DIR}/src/trusted.cpp ${EXTERN_DIR}/src/untrusted.cpp ${EXTERN_DIR}/src/training.cpp ${EXTERN_DIR}/src/repository.cpp ${UTIL_SRCS} ${KMEANS_SRCS})

add_library(${EXTERN_LIB_NAME} ${SOURCES})
target_include_directories(${EXTERN_LIB_NAME} PRIVATE ${EXTERN_DIR} ${EXTERN_INCLUDE_DIR} ${FRAMEWORK_INCLUDE_DIR} ${SGX_INCLUDE_PATHS})#TODO extern include dir still needed?
