# flags
set(CMAKE_C_FLAGS ${sgx_c_flags})
set(CMAKE_CXX_FLAGS ${sgx_cpp_flags})

add_library(${LIB_TRUSTED} ${LIB_TRUSTED_DIR}/trusted_util.cpp ${LIB_TRUSTED_DIR}/trusted_crypto.cpp ${LIB_TRUSTED_DIR}/outside_util.cpp)

target_include_directories(${LIB_TRUSTED} PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${IEE_COMM_DIR}/ocall)
target_link_libraries(${LIB_TRUSTED} sodium)
