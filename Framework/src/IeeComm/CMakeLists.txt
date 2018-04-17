# flags
set(c_flags "${SGX_COMMON_CFLAGS} -fPIC -Wall -Wno-attributes -DDEBUG -UNDEBUG -UEDEBUG")
set(CMAKE_C_FLAGS ${c_flags})
set(CMAKE_CXX_FLAGS ${c_flags})

add_custom_command(
    OUTPUT ${IEE_COMM_DIR}/Enclave_u.h ${IEE_COMM_DIR}/Enclave_u.c
    COMMAND ${SGX_EDGER8R} --untrusted ${IEE_DIR}/Enclave.edl --untrusted-dir ${IEE_COMM_DIR} --search-path ${IEE_DIR} --search-path ${FRAMEWORK_INCLUDE_DIR} --search-path ${SGX_SDK}/include
)

add_custom_target(edgr8r_untrusted ALL DEPENDS ${IEE_COMM_DIR}/Enclave_u.h ${IEE_COMM_DIR}/Enclave_u.c)

link_directories(${SGX_LIBRARY_PATH} ${EXTERN_DIR}/lib)

#find_library(URTS sgx_urts PATHS /usr/lib NO_CMAKE_FIND_ROOT_PATH)
#find_library(UAE_SERVICE sgx_uae_service PATHS /usr/lib NO_CMAKE_FIND_ROOT_PATH)

add_executable(IeeComm ${IEE_COMM_DIR}/App.cpp ${IEE_COMM_DIR}/Enclave_u.c ${IEE_COMM_DIR}/ocall/ocall.cpp ${IEE_COMM_DIR}/sgx_handler/sgx_handler.cpp)

target_include_directories(IeeComm PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${EXTERN_INCLUDE_DIR} ${IEE_COMM_DIR} ${IEE_COMM_DIR}/ocall ${IEE_COMM_DIR}/sgx_handler ${LIBUTIL_INCLUDE_DIR} ${IEE_COMM_DIR}/../${IEE_DIR} ${SGX_SDK}/include)
target_link_libraries(IeeComm Util sgx_urts pthread sgx_uae_service ${EXTERN_LIB_NAME})
