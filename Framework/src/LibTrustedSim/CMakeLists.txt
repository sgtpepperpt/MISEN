# flags
set(COMMON_FLAGS "-pthread -g -O0 -fPIC -Wall -Wno-unused-function -Wno-unused-variable -Wno-sign-compare")
set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS}")

add_library(${LIB_TRUSTED} ${LIB_TRUSTED_SIM_DIR}/sim_trusted_crypto.cpp ${LIB_TRUSTED_SIM_DIR}/sim_trusted_util.cpp ${LIB_TRUSTED_SIM_DIR}/sim_outside_util.cpp)

target_include_directories(${LIB_TRUSTED} PRIVATE ${FRAMEWORK_INCLUDE_DIR})
target_link_libraries(${LIB_TRUSTED} ${LIB_UNTRUSTED})
