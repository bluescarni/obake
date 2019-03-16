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
# NOTE: stack traces are only supported on linux/mingw or MSVC at the moment.
if (YACMA_COMPILER_IS_MSVC OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR MINGW)
    option(PIRANHA_WITH_STACK_TRACES "Enable stack traces." OFF)
endif()
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
    # Enable strict conformance mode.
    list(APPEND PIRANHA_CXX_FLAGS_DEBUG "/permissive-")
    list(APPEND PIRANHA_CXX_FLAGS_RELEASE "/permissive-")
endif()

# Find the dependencies.

# mp++.
find_package(mp++ 0.13 REQUIRED)

# libbacktrace.
# NOTE: if stack traces are not supported, then the PIRANHA_WITH_STACK_TRACES
# variable is empty and evaluates to false.
if(PIRANHA_WITH_STACK_TRACES)
    set(PIRANHA_ENABLE_STACK_TRACES "#define PIRANHA_WITH_STACK_TRACES")
    if(NOT YACMA_COMPILER_IS_MSVC)
        find_package(libbacktrace REQUIRED)
    endif()
endif()

# DbgHelp. This is already pulled in by mp++, but for consistency we do it again.
if(YACMA_COMPILER_IS_MSVC)
    find_package(DbgHelp REQUIRED)
endif()

# Initial setup of the piranha target.
set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/demangle.cpp")

if(PIRANHA_WITH_STACK_TRACES)
    if(YACMA_COMPILER_IS_MSVC)
        set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/msvc_stack_trace.cpp" "${PIRANHA_SRC_FILES}")
    else()
        set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/libbacktrace_stack_trace.cpp" "${PIRANHA_SRC_FILES}")
    endif()
endif()

add_library(piranha SHARED "${PIRANHA_SRC_FILES}")

target_compile_options(piranha PRIVATE "$<$<CONFIG:DEBUG>:${PIRANHA_CXX_FLAGS_DEBUG}>" "$<$<CONFIG:RELEASE>:${PIRANHA_CXX_FLAGS_RELEASE}>")
set_target_properties(piranha PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(piranha PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
set_property(TARGET piranha PROPERTY CXX_STANDARD 17)
set_property(TARGET piranha PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET piranha PROPERTY CXX_EXTENSIONS NO)

# Add the include dirs to the piranha target's interface.
target_include_directories(piranha PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Link the deps.
target_link_libraries(piranha PUBLIC mp++::mp++)
if(YACMA_COMPILER_IS_MSVC)
    target_link_libraries(piranha PUBLIC DbgHelp::DbgHelp)
endif()
if(PIRANHA_WITH_STACK_TRACES AND NOT YACMA_COMPILER_IS_MSVC)
    target_link_libraries(piranha PUBLIC libbacktrace::libbacktrace)
endif()

# Configure config.hpp.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" @ONLY)

if(PIRANHA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
