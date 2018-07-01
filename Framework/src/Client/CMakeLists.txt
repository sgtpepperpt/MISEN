find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

link_directories(${MBEDTLS_DIR}/build/library)

add_executable(Client ${CLIENT_DIR}/Client.cpp)
target_include_directories(Client PRIVATE ${CLIENT_DIR} ${FRAMEWORK_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${MBEDTLS_DIR}/include)
target_link_libraries(Client ${LIB_UNTRUSTED} ${LIB_UNTRUSTED_TLS} ${OpenCV_LIBS} ${OPENSSL_CRYPTO_LIBRARY} mbedcrypto mbedtls mbedx509)
