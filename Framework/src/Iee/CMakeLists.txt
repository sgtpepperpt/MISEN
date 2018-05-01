set(ENCLAVE_KEY_FILE ${IEE_DIR}/sgx_config/Enclave_private.pem)
set(ENCLAVE_CONFIG_FILE ${IEE_DIR}/sgx_config/Enclave.config.xml)
set(ENCLAVE_LINKER_FILE ${IEE_DIR}/sgx_config/Enclave_debug.lds)

# flags
set(CMAKE_C_FLAGS ${sgx_c_flags})
set(CMAKE_CXX_FLAGS ${sgx_cpp_flags})

add_custom_command(
    OUTPUT ${IEE_DIR}/Enclave_t.h ${IEE_DIR}/Enclave_t.c
    COMMAND ${SGX_EDGER8R} --trusted-dir ${IEE_DIR} --trusted ${IEE_DIR}/Enclave.edl --search-path ${IEE_DIR} --search-path ${FRAMEWORK_INCLUDE_DIR} --search-path $(SGX_SDK)/include
    DEPENDS ${IEE_DIR}/Enclave.edl
)
add_custom_target(edger8r_trusted ALL DEPENDS ${IEE_DIR}/Enclave_t.h ${IEE_DIR}/Enclave_t.c)

link_directories(${SGX_LIBRARY_PATH})

# sources
set(IEE_ENCLAVE ${IEE_DIR}/Enclave.cpp ${IEE_DIR}/Enclave_t.c)
set(IEE_THREADING ${IEE_DIR}/thread_handler.cpp)

add_executable(${ENCLAVE_NAME} ${IEE_ENCLAVE} ${IEE_THREADING})
add_dependencies(${ENCLAVE_NAME} edger8r_trusted)

# includes and linking
target_include_directories(${ENCLAVE_NAME} PRIVATE ${FRAMEWORK_INCLUDE_DIR} ${EXTERN_INCLUDE_DIR} ${IEE_DIR} ${IEE_DIR}/ocall_wrappers ${IEE_DIR}/threading ${SGX_INCLUDE_PATHS})

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
target_link_libraries(${ENCLAVE_NAME} -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles
    -Wl,--whole-archive sgx_trts -Wl,--no-whole-archive
    -Wl,--start-group sgx_tstdc sgx_tcxx sgx_tcrypto sgx_tservice sgx_tprotected_fs ${LIB_TRUSTED} ${EXTERN_LIB_NAME} -Wl,--end-group
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic
    -Wl,--defsym,__ImageBase=0
    -Wl,--version-script=${ENCLAVE_LINKER_FILE}
    ${EXTERN_LIB_NAME}
)

add_custom_command(
    OUTPUT enclave
    COMMAND ${SGX_ENCLAVE_SIGNER} sign -key ${ENCLAVE_KEY_FILE} -enclave ${EXECUTABLE_OUTPUT_PATH}/${ENCLAVE_NAME} -out ${EXECUTABLE_OUTPUT_PATH}/${SIGNED_ENCLAVE_NAME} -config ${ENCLAVE_CONFIG_FILE}
)

add_custom_target(signer ALL DEPENDS enclave)

