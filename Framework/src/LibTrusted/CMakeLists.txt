# flags
set(CMAKE_C_FLAGS ${sgx_c_flags})
set(CMAKE_CXX_FLAGS ${sgx_cpp_flags})

add_library(${LIB_TRUSTED} ${LIB_TRUSTED_DIR}/trusted_crypto.cpp ${LIB_TRUSTED_DIR}/trusted_util.cpp ${LIB_TRUSTED_DIR}/outside_util.cpp)
add_dependencies(${LIB_TRUSTED} edger8r_trusted)

target_include_directories(${LIB_TRUSTED} PRIVATE /usr/local/include ${FRAMEWORK_INCLUDE_DIR} ${SGX_INCLUDE_PATHS} ${IEE_DIR}) # ${IEE_DIR} for enclave_t
target_link_libraries(${LIB_TRUSTED} sodium-sgx)
