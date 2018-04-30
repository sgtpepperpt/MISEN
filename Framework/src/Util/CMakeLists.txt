set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(SOURCES ${LIBUTIL_DIR}/untrusted_util.cpp ${LIBUTIL_DIR}/untrusted_crypto.cpp)
add_library(Util ${SOURCES})
target_include_directories(Util PRIVATE ${FRAMEWORK_INCLUDE_DIR})
