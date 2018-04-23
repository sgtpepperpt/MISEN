# flags
set(COMMON_FLAGS "-pthread -g -O0 -fPIC -Wall -Wno-unused-function -Wno-unused-variable -Wno-sign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# lib util
set(LIBUTIL_DIR ${FRAMEWORK_DIR}/src/Util)
add_subdirectory(${LIBUTIL_DIR})

# uee module
set(SERVER_DIR ${FRAMEWORK_DIR}/src/Uee)
add_subdirectory(${SERVER_DIR})

# compile client module
set(CLIENT_DIR ${FRAMEWORK_DIR}/src/Client)
add_subdirectory(${CLIENT_DIR})

######################################################### SGX #########################################################
# define common sgx vars
if (NOT DEFINED ENV{SGX_SDK})
    set(SGX_SDK /opt/intel/sgxsdk)
else()
    set(SGX_SDK $ENV{SGX_SDK})
endif ()

set(SGX_COMMON_CFLAGS "-m64 -O0 -g")
set(SGX_LIBRARY_PATH ${SGX_SDK}/lib64)
set(SGX_EDGER8R ${SGX_SDK}/bin/x64/sgx_edger8r)
set(SGX_ENCLAVE_SIGNER ${SGX_SDK}/bin/x64/sgx_sign)

# dirs
set(IEE_COMM_DIR ${FRAMEWORK_DIR}/src/IeeComm)
set(IEE_DIR ${FRAMEWORK_DIR}/src/Iee)

# filenames for enclave
set(ENCLAVE_NAME enclave.so)
set(SIGNED_ENCLAVE_NAME enclave.signed.so)
add_definitions(-DSIGNED_ENCLAVE_NAME="${SIGNED_ENCLAVE_NAME}")
add_definitions(-DENCLAVE_TOKEN_NAME="enclave.token")

# compile iee_comm and iee modules
add_subdirectory(${FRAMEWORK_DIR}/src/IeeComm)
add_subdirectory(${FRAMEWORK_DIR}/src/Iee)
