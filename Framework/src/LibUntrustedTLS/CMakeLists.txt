set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

link_directories(${MBEDTLS_DIR}/build/library)

set(SOURCES ${LIB_UNTRUSTED_TLS_DIR}/untrusted_util_tls.cpp)
add_library(${LIB_UNTRUSTED_TLS} ${SOURCES})

target_include_directories(${LIB_UNTRUSTED_TLS} PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${MBEDTLS_DIR}/include)
target_link_libraries(${LIB_UNTRUSTED_TLS} mbedcrypto mbedtls mbedx509)
