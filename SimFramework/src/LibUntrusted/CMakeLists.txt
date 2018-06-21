set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(SOURCES ${LIB_UNTRUSTED_DIR}/untrusted_util.cpp ${LIB_UNTRUSTED_DIR}/untrusted_crypto.cpp)
add_library(${LIB_UNTRUSTED} ${SOURCES})
target_include_directories(${LIB_UNTRUSTED} PRIVATE ${FRAMEWORK_INCLUDE_DIR})
target_link_libraries(${LIB_UNTRUSTED} sodium)
