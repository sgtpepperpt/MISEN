set(CMAKE_C_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_CXX_FLAGS "-pthread -g -O0 -fPIC -Wall")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_executable(Uee ${SERVER_DIR}/SseServer.cpp)
target_include_directories(Uee PRIVATE ${LIBUTIL_INCLUDE_DIR} ${SERVER_DIR})
target_link_libraries(Uee Util)
