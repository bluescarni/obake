cmake_minimum_required(VERSION 3.3.0)

project(piranha VERSION 0.19 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/yacma")

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "piranha version: ${piranha_VERSION}")

# Set default build type to "Release".
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif()

# The build options.
option(PIRANHA_BUILD_TESTS "Build unit tests." OFF)

# Run the YACMA compiler setup.
include(YACMACompilerLinkerSettings)

# Assemble the flags.
set(PIRANHA_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG})
set(PIRANHA_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS})

if(YACMA_COMPILER_IS_MSVC)
    # Disable the idiotic minmax macros on MSVC.
    list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-DNOMINMAX")
    list(APPEND PIRANHA_CXX_FLAGS_RELEASE "-DNOMINMAX")
endif()

# Find the dependencies.

# mp++.
find_package(mp++ 0.13 REQUIRED)

# Initial setup of the piranha target.
add_library(piranha INTERFACE)

# Add the include dirs to the piranha target's interface.
target_include_directories(piranha INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Link the deps.
target_link_libraries(piranha INTERFACE mp++::mp++)

# Configure config.hpp.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" @ONLY)

if(PIRANHA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
