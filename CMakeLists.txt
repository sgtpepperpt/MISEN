cmake_minimum_required(VERSION 3.5)
project(sgx_image_search)

# process arguments
if(NOT DEFINED SIMULATED_MODE)
    set(SIMULATED_MODE 0)
endif()

if(SIMULATED_MODE)
    set(FRAMEWORK_NAME SimFramework)
else()
    set(FRAMEWORK_NAME Framework)
endif()

# base paths
set(MBEDTLS_DIR /home/guilherme/libs/mbedtls-SGX/build/mbedtls_SGX-2.6.0)
set(FRAMEWORK_DIR ${CMAKE_SOURCE_DIR}/${FRAMEWORK_NAME})
set(EXTERN_DIR ${CMAKE_SOURCE_DIR}/ImageSearch)
set(EXTERN_LIB_NAME ImageSearch)

########################################################################################################################
########################################### PROBABLY NOT NEEDED TO EDIT BELOW ##########################################
########################################################################################################################

# paths for binaries
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

# pass vars to program
add_definitions(-DSIMULATED_MODE=${SIMULATED_MODE})

# include dirs
set(FRAMEWORK_INCLUDE_DIR ${FRAMEWORK_DIR}/include)
set(EXTERN_INCLUDE_DIR ${EXTERN_DIR}/include) # not used

add_subdirectory(${EXTERN_DIR})
add_subdirectory(${FRAMEWORK_DIR})
