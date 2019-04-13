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
option(PIRANHA_WITH_LIBBACKTRACE "Use libbacktrace for improved stack traces." OFF)

# Run the YACMA compiler setup.
include(YACMACompilerLinkerSettings)

# Assemble the flags.
set(PIRANHA_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG})
set(PIRANHA_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS})

if(YACMA_COMPILER_IS_MSVC)
    # Disable the idiotic minmax macros on MSVC.
    list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-DNOMINMAX")
    list(APPEND PIRANHA_CXX_FLAGS_RELEASE "-DNOMINMAX")
    # Enable strict conformance mode, if supported.
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_cxx_compiler_flag("/permissive-" _PIRANHA_MSVC_SUPPORTS_STRICT_CONFORMANCE)
    unset(CMAKE_REQUIRED_QUIET)
    if(_PIRANHA_MSVC_SUPPORTS_STRICT_CONFORMANCE)
        message(STATUS "The '/permissive-' flag is supported, enabling it.")
        list(APPEND PIRANHA_CXX_FLAGS_DEBUG "/permissive-")
        list(APPEND PIRANHA_CXX_FLAGS_RELEASE "/permissive-")
    endif()
    unset(_PIRANHA_MSVC_SUPPORTS_STRICT_CONFORMANCE)
endif()

# Find the dependencies.

# mp++.
find_package(mp++ 0.14 REQUIRED)

# Boost setup.
include(PiranhaFindBoost)

# libbacktrace.
if(PIRANHA_WITH_LIBBACKTRACE)
    set(PIRANHA_ENABLE_LIBBACKTRACE "#define PIRANHA_WITH_LIBBACKTRACE")
    find_package(libbacktrace REQUIRED)
endif()

# Initial setup of the piranha target.
set(PIRANHA_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/bit_packing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/to_string.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/stack_trace.cpp"
)

if(PIRANHA_WITH_LIBBACKTRACE)
    set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/libbacktrace_stack_trace.cpp" "${PIRANHA_SRC_FILES}")
else()
    set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/boost_stack_trace.cpp" "${PIRANHA_SRC_FILES}")
endif()

add_library(piranha SHARED "${PIRANHA_SRC_FILES}")
set_property(TARGET piranha PROPERTY VERSION "${piranha_VERSION_MAJOR}.${piranha_VERSION_MINOR}")
set_property(TARGET piranha PROPERTY SOVERSION 0 )

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
if(PIRANHA_WITH_LIBBACKTRACE)
    target_link_libraries(piranha PUBLIC libbacktrace::libbacktrace)
else()
    # NOTE: Boost stacktrace sometimes requires linking to the system
    # DL libs. There's a CMake variable for that, which I hope
    # behaves gracefully on non-unix platforms.
    # NOTE: when we enable piranha's installation machinery,
    # we may have to sort this out (CMAKE_DL_LIBS does not
    # look like an imported target).
    target_link_libraries(piranha PUBLIC ${CMAKE_DL_LIBS})
endif()

# Configure config.hpp.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" @ONLY)

# Configure the sphinx config file.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py.in" "${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py" @ONLY)

if(PIRANHA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
