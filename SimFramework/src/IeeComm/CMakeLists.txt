# flags
set(c_flags "${sgx_common_flags} -fPIC -Wall -Wno-attributes")
set(CMAKE_C_FLAGS ${c_flags})
set(CMAKE_CXX_FLAGS ${c_flags})

#link_directories(${SGX_LIBRARY_PATH})

#find_library(URTS sgx_urts PATHS /usr/lib NO_CMAKE_FIND_ROOT_PATH)
#find_library(UAE_SERVICE sgx_uae_service PATHS /usr/lib NO_CMAKE_FIND_ROOT_PATH)

add_executable(IeeComm ${IEE_COMM_DIR}/App.cpp ${IEE_COMM_DIR}/ocall/ocall.cpp ${IEE_COMM_DIR}/threading/thread_pool.cpp)

target_include_directories(IeeComm PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${EXTERN_INCLUDE_DIR} ${IEE_DIR} ${IEE_COMM_DIR} ${IEE_COMM_DIR}/ocall ${IEE_COMM_DIR}/threading ${IEE_COMM_DIR}/../${IEE_DIR})
target_link_libraries(IeeComm ${LIB_UNTRUSTED} pthread ${ENCLAVE_NAME})
