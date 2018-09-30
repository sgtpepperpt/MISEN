# flags
set(COMMON_FLAGS "-pthread -g -O0 -fPIC -Wall -Wno-unused-function -Wno-unused-variable -Wno-sign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

######################################################### SGX #########################################################
# filenames for enclave
set(ENCLAVE_NAME enclave.so)
set(SIGNED_ENCLAVE_NAME enclave.signed.so)
add_definitions(-DSIGNED_ENCLAVE_NAME="${SIGNED_ENCLAVE_NAME}")
add_definitions(-DENCLAVE_TOKEN_NAME="enclave.token")

# sgx flags
set(sgx_common_flags "-m64 -O0 -g")
set(sgx_c_flags "${sgx_common_flags} -fvisibility=hidden -fpie -fstack-protector -ffunction-sections -fdata-sections -fstack-protector-strong")
set(sgx_cpp_flags "${sgx_c_flags}")

######################################################### SGX #########################################################

# directories for modules
set(LIB_UNTRUSTED_DIR ${FRAMEWORK_DIR}/src/LibUntrusted)
set(LIB_TRUSTED_DIR ${FRAMEWORK_DIR}/src/LibTrusted)
set(STORAGE_DIR ${FRAMEWORK_DIR}/src/StorageService)
set(CLIENT_DIR ${FRAMEWORK_DIR}/src/Client)
set(IEE_COMM_DIR ${FRAMEWORK_DIR}/src/IeeComm)
set(IEE_DIR ${FRAMEWORK_DIR}/src/Iee)

# lib untrusted
set(LIB_UNTRUSTED untrusted_util)
add_subdirectory(${LIB_UNTRUSTED_DIR})

# lib trusted
set(LIB_TRUSTED trusted_util)
add_subdirectory(${LIB_TRUSTED_DIR})

# uee module
add_subdirectory(${STORAGE_DIR})

# client module
add_subdirectory(${CLIENT_DIR})

# iee module
add_subdirectory(${IEE_DIR})

# iee_comm module
add_subdirectory(${IEE_COMM_DIR})
