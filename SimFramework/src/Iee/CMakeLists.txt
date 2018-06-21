# flags
set(CMAKE_C_FLAGS ${sgx_c_flags})
set(CMAKE_CXX_FLAGS ${sgx_cpp_flags})

#link_directories(${SGX_LIBRARY_PATH})

add_library(${ENCLAVE_NAME} ${IEE_DIR}/Enclave.cpp ${IEE_DIR}/thread_handler.cpp)

# includes and linking
target_include_directories(${ENCLAVE_NAME} PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${EXTERN_INCLUDE_DIR} ${IEE_DIR} ${IEE_DIR}/ocall_wrappers ${IEE_DIR}/threading ${SGX_INCLUDE_PATHS})
target_link_libraries(${ENCLAVE_NAME} ${LIB_TRUSTED} ${EXTERN_LIB_NAME})
