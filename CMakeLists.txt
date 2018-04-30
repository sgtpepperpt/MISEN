cmake_minimum_required(VERSION 3.5)
project(sgx_image_search)

#set(SIMULATED_MODE 0)

# base paths
set(FRAMEWORK_DIR ${CMAKE_SOURCE_DIR}/Framework)
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
#add_definitions(-DSIMULATED_MODE=${SIMULATED_MODE})

# include dirs
set(FRAMEWORK_INCLUDE_DIR ${FRAMEWORK_DIR}/include)
set(EXTERN_INCLUDE_DIR ${EXTERN_DIR}/include)

add_subdirectory(${EXTERN_DIR})
add_subdirectory(${FRAMEWORK_DIR})
