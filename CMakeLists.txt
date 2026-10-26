cmake_minimum_required(VERSION 3.16)
project(fishnet_project VERSION 2.0.1 LANGUAGES CXX)

# Options
option(FISHNET_APPS "Build common fishnet applications" ON)
option(FISHNET_EXAMPLES "Build example applications" ON)
option(FISHNET_TEST "Enable unit tests for Fishnet components" ON)
option(FISHNET_TEST_LIB "Build Fishnet test utility library" ON)
option(FISHNET_COVERAGE "Enable coverage reporting" OFF)
option(FISHNET_COMPILE_TIME_TRACE "Enable compile time tracing" OFF)
option(FISHNET_DOCS "Enable documentation generation" OFF)

# Variables
set(FISHNET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Build applications")
set(FISHNET_NAME Fishnet)
set(FISHNET ${FISHNET_NAME})
set(CMAKE_CXX_STANDARD 23)

# Compiler flags and warnings
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Setting compiler flags and warnings")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter -Wno-unused-function)
    if(FISHNET_COMPILE_TIME_TRACE)
        add_compile_options(-fplugin=externis -fplugin-arg-externis-trace=${CMAKE_BINARY_DIR}/trace.json)
    endif()
endif()

# Build core lib
add_subdirectory(lib)

# Build common applications
if(FISHNET_APPS)
    add_subdirectory(src)
endif()

# Build example applications
if(FISHNET_EXAMPLES)
    add_subdirectory(example)
endif()

# Enable code coverage
if(FISHNET_COVERAGE)
    # Add coverage compile flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage -fprofile-update=atomic")
    link_libraries(gcov)
endif()

# Build tests
if(FISHNET_TEST)
    include(extern/gtest.cmake)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
if(FISHNET_DOCS)
    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating HTML documentation with Doxygen"
            VERBATIM)
        message(STATUS "Doxygen found: docs target available (run 'cmake --build . --target docs')")
    else()
        message(STATUS "Doxygen not found, documentation target unavailable")
    endif()
endif()
