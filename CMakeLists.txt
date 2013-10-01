cmake_minimum_required (VERSION 2.8.0)
project (VeilClient C CXX)

include(ExternalProject)

# The version number.
set(VeilClient_VERSION_MAJOR 0)
set(VeilClient_VERSION_MINOR 3)
set(VeilClient_VERSION_PATCH 0)
set(VeilClient_VERSION $(VeilClient_VERSION_MAJOR).$(VeilClient_VERSION_MINOR).$(VeilClient_VERSION_PATCH))

# Install dirs
set(CONFIG_DIR "etc")
set(BIN_DIR "bin")
set(LIB_DIR "lib")

# CMake config
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
set(DEPS_DIR "deps")
# set(CMAKE_FIND_LIBRARY_SUFFIXES .a) # Uncomment in order to force looking for static achives instead .so
configure_file (
    "${PROJECT_SOURCE_DIR}/include/veilConfig.h.in"
    "${PROJECT_BINARY_DIR}/veilConfig.h"
)

# Deps
add_subdirectory(${DEPS_DIR}/glog)
add_subdirectory(${DEPS_DIR}/yaml-cpp)
add_subdirectory(${DEPS_DIR}/gmock)
add_subdirectory(${DEPS_DIR}/veilhelpers)

link_directories(${PROJECT_BINARY_DIR})


# Include paths
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/fuse)
include_directories(${PROJECT_BINARY_DIR})
include_directories(${GLOG_INCLUDE_DIR})
include_directories(${YAML_INCLUDE_DIR})
include_directories(${VEILHELPERS_INCLUDE_DIR})


# Setup fuse
message(STATUS "Checking for FUSE...")
find_package(FUSE REQUIRED)
include_directories(${FUSE_INCLUDE_DIRS})
message(STATUS "FUSE: ${FUSE_LIBRARIES}")
set(FUSE_LIBRARIES "fuse")  

# Setup ProtoBuf
message(STATUS "Checking for Protobuf...")
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

# Setup pthreads
message(STATUS "Checking for pthreads...")
find_package(Threads REQUIRED)

# Setup git
message(STATUS "Checking for Git...")
find_package(Git REQUIRED)

# Setup OpenSSL
message(STATUS "Checking for OpenSSL...")
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Setup Boost
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS system thread filesystem REQUIRED)
set(Boost_LIBRARIES "boost_thread-mt.a;boost_filesystem-mt.a;boost_system-mt.a") # SL6 bug fix
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

# Pull git submodules
message(STATUS "Pulling submodules using git...")
execute_process(COMMAND ${GIT_EXECUTABLE} submodule init
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
execute_process(COMMAND ${GIT_EXECUTABLE} submodule foreach git checkout 
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Compile .proto files
file(GLOB PROTO_FILES "${PROJECT_SOURCE_DIR}/veilprotocol/proto/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
include_directories(${CMAKE_CURRENT_BINARY_DIR}) 

# Setup sources
file(GLOB SOURCES "src/[a-zA-Z]*.cc")
file(GLOB MAIN_SOURCE_FILE "src/veilFuse.cc")
string(REGEX REPLACE "${MAIN_SOURCE_FILE}" "" SOURCES "${SOURCES}")

# Setup compile flags
set(CUSTOM_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}:${CMAKE_CURRENT_BINARY_DIR}:.:${LIB_DIR}:../${LIB_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ${FUSE_DEFINITIONS} -Wl,-rpath,${CUSTOM_RPATH}")
set(CMAKE_SKIP_RPATH TRUE) # Something is wrong with CMake RPATH config, we need to use old-fashion way

# Define targets

add_library(veilprotocol ${PROTO_SRCS})
add_library(veilclient ${SOURCES})
add_executable(veilFuse ${MAIN_SOURCE_FILE})

add_dependencies(veilFuse yaml-cpp glog veilclient) 
add_dependencies(veilclient yaml-cpp glog veilprotocol veilhelpers) 
set_target_properties(veilFuse PROPERTIES COMPILE_FLAGS "-Wall -g")

target_link_libraries(veilprotocol
    ${PROTOBUF_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(veilclient
    ${FUSE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${GLOG_LIBRARIES}
    ${YAML_LIBRARIES}
    ${VEILHELPERS_LIBRARIES}
    veilprotocol
    dl
    z
    rt
)

target_link_libraries(veilFuse 
    ${GLOG_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${FUSE_LIBRARIES}
    ${PROTOBUF_LIBRARY}
    ${VEILHELPERS_LIBRARIES}
    veilclient
    dl
    z
    rt
)

# Install config
install(TARGETS veilFuse
        RUNTIME DESTINATION ${BIN_DIR}
        LIBRARY DESTINATION ${LIB_DIR}
)
install(DIRECTORY config/ DESTINATION ${CONFIG_DIR})

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_SET_DESTDIR ON) # Uncomment this line if you need a non-relocatable RPM package
set(CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${VeilClient_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VeilClient_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "help@veilfs.com")

# DEB and RPM deps 
set(CPACK_DEBIAN_PACKAGE_DEPENDS "fuse, libfuse2, libprotobuf7, libssl1.0.0 globus-proxy-utils")
set(CPACK_RPM_PACKAGE_REQUIRES "fuse, fuse-libs, protobuf openssl globus-proxy-utils")
set(CPACK_RPM_PACKAGE_AUTOREQPROV " no")

include(CPack)

###########################
##         TESTS         ##
###########################

enable_testing()
include_directories(${GTEST_INCLUDE_DIR})
include_directories(${GMOCK_INCLUDE_DIR})

set(TEST_SOURCES_DIR "${PROJECT_SOURCE_DIR}/test")
include_directories(${TEST_SOURCES_DIR}/include)
include_directories(${VEILHELPERS_TEST_INCLUDE_DIR})

# Unit tests

add_custom_target(cunit)
macro(run_test test_target)
  string(REGEX REPLACE "_test" "" test_name "${test_target}")  
  add_custom_target(${test_target}_runtest
      COMMAND ${test_target} --gtest_output="xml:cunit_results/TEST-${test_name}.xml"
      DEPENDS ${test_target}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
  add_dependencies(cunit ${test_target}_runtest)
endmacro()

file(GLOB files ${TEST_SOURCES_DIR}/unit_tests/*_test.cc)
foreach(file ${files})
    get_filename_component(TEST_NAME ${file} NAME_WE)
    add_executable(${TEST_NAME} ${file})
    add_dependencies(${TEST_NAME} gtest gmock veilFuse veilhelpers veilclient veilprotocol)
    target_link_libraries(${TEST_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${FUSE_LIBRARIES}
        ${PROTOBUF_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${VEILHELPERS_LIBRARIES}
        veilclient
        dl 
        z
    )    
    add_test(${TEST_NAME} ${TEST_NAME})
    run_test(${TEST_NAME})
endforeach()


# Integration tests

set(TEST_UTILS_DIR ${TEST_SOURCES_DIR}/erl_core)
add_library(erlTestCore ${TEST_UTILS_DIR}/erlTestCore.cc)

file(GLOB erlc_files ${TEST_SOURCES_DIR}/*/*.erl)
add_custom_target(build_erl_core
    COMMAND erlc -I ${TEST_UTILS_DIR} ${erlc_files}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_custom_target(integration_tests)
macro(run_test_i test_target)
  string(REGEX REPLACE "_test" "" test_name "${test_target}")
  string(REGEX REPLACE "_test_i" "_test" beam_name "${test_target}")   
  add_custom_target(${test_target}_runtest_i
      COMMAND ${TEST_UTILS_DIR}/run_test.escript ${beam_name} --gtest_output="xml:integration_results/TEST-${test_name}.xml"
      DEPENDS ${test_target} build_erl_core
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
  add_dependencies(integration_tests ${test_target}_runtest_i)
endmacro()

file(GLOB files ${TEST_SOURCES_DIR}/integration_tests/*_test.cc)
foreach(file ${files})
    get_filename_component(TEST_NAME ${file} NAME_WE)
    set(TEST_NAME "${TEST_NAME}_i")
    add_executable(${TEST_NAME} ${file} ${TEST_UTILS_DIR}/erlTestCore.cc)
    add_dependencies(${TEST_NAME} gtest gmock veilFuse veilhelpers veilclient veilprotocol)
    target_link_libraries(${TEST_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        ${GTEST_LIBRARIES}
        ${GMOCK_LIBRARIES}
        ${FUSE_LIBRARIES}
		${PROTOBUF_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${VEILHELPERS_LIBRARIES}
        veilclient
        dl 
        z
        erlTestCore
    )    
    run_test_i(${TEST_NAME})
endforeach()