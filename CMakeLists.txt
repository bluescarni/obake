cmake_minimum_required(VERSION 3.3.0)

# Set default build type to "Release".
# NOTE: this should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif()

# NOTE: piranha is C++, however we are linking to C
# libraries (e.g., libbacktrace). Moreover, if we don't
# enable C here, on some configurations (e.g., Ubuntu
# + clang) the threading detection/setup seems to break
# down.
project(piranha VERSION 0.19 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/yacma")

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "piranha version: ${piranha_VERSION}")

# The build options.
option(PIRANHA_BUILD_TESTS "Build unit tests." OFF)
option(PIRANHA_BUILD_BENCHMARKS "Build benchmarks." OFF)
option(PIRANHA_WITH_LIBBACKTRACE "Use libbacktrace for improved stack traces." OFF)

# Run the YACMA compiler setup.
include(YACMACompilerLinkerSettings)
# Also the threading setup.
include(YACMAThreadingSetup)

# NOTE: on Unix systems, the correct library installation path
# could be something other than just "lib", such as "lib64",
# "lib32", etc., depending on platform/configuration. Apparently,
# CMake provides this information via the GNUInstallDirs module.
# Let's enable this for now on all Unixes except OSX.
# NOTE: potentially, this could be applicable to Cygwin as well.
#
# https://cmake.org/cmake/help/v3.15/module/GNUInstallDirs.html
# https://cmake.org/pipermail/cmake/2013-July/055375.html
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(_PIRANHA_INSTALL_LIBDIR_DEFAULT "${CMAKE_INSTALL_LIBDIR}")
else()
    set(_PIRANHA_INSTALL_LIBDIR_DEFAULT "lib")
endif()
if(NOT PIRANHA_INSTALL_LIBDIR)
    set(PIRANHA_INSTALL_LIBDIR "${_PIRANHA_INSTALL_LIBDIR_DEFAULT}" CACHE STRING
        "Library installation directory." FORCE)
endif()
mark_as_advanced(PIRANHA_INSTALL_LIBDIR)
message(STATUS "Library installation directory: ${PIRANHA_INSTALL_LIBDIR}")

# Assemble the flags.
set(PIRANHA_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG})
set(PIRANHA_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS})

if(YACMA_COMPILER_IS_MSVC)
    # Disable the idiotic minmax macros on MSVC (both cl and clang-cl).
    # Also, enable the bigobj flag and the WIN32_LEAN_AND_MEAN definitions:
    # https://stackoverflow.com/questions/11040133/what-does-defining-win32-lean-and-mean-exclude-exactly
    list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-DNOMINMAX" "/bigobj" "-DWIN32_LEAN_AND_MEAN")
    list(APPEND PIRANHA_CXX_FLAGS_RELEASE "-DNOMINMAX" "/bigobj" "-DWIN32_LEAN_AND_MEAN")
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
    if(YACMA_COMPILER_IS_CLANGXX)
        # clang-cl emits various warnings from GMP/MPFR, let's just silence them.
        # NOTE: at one point in the recent past, MSVC added an options similar to GCC's isystem:
        # https://blogs.msdn.microsoft.com/vcblog/2017/12/13/broken-warnings-theory/
        # We probably just need to wait for this to be picked up by CMake/clang-cl. Let's
        # revisit the issue in the future.
        list(APPEND _PIRANHA_CLANG_CL_DISABLED_WARNINGS
            "-Wno-unused-variable"
            "-Wno-inconsistent-dllimport"
            "-Wno-unknown-pragmas"
            "-Wno-unused-parameter"
            "-Wno-sign-compare"
            "-Wno-deprecated-declarations"
            "-Wno-deprecated-dynamic-exception-spec"
            "-Wno-old-style-cast"
            "-Wno-sign-conversion"
            "-Wno-non-virtual-dtor"
            "-Wno-deprecated"
            "-Wno-shadow"
            "-Wno-shorten-64-to-32"
            "-Wno-reserved-id-macro"
            "-Wno-undef"
            "-Wno-c++98-compat-pedantic"
            "-Wno-documentation-unknown-command"
            "-Wno-zero-as-null-pointer-constant"
            "-Wno-language-extension-token"
            "-Wno-gnu-anonymous-struct"
            "-Wno-nested-anon-types"
            "-Wno-documentation"
            "-Wno-comma"
            "-Wno-nonportable-system-include-path"
            "-Wno-global-constructors"
            "-Wno-redundant-parens"
            "-Wno-exit-time-destructors"
            "-Wno-missing-noreturn"
            "-Wno-switch-enum"
            "-Wno-covered-switch-default"
            "-Wno-float-equal"
            "-Wno-double-promotion"
            "-Wno-microsoft-enum-value"
            "-Wno-missing-prototypes"
            "-Wno-implicit-fallthrough"
            "-Wno-format-nonliteral"
            "-Wno-cast-qual"
            "-Wno-disabled-macro-expansion"
            "-Wno-unused-private-field"
            "-Wno-unused-template"
            "-Wno-unused-macros"
            "-Wno-extra-semi-stmt"
            "-Wno-c++98-compat")
        list(APPEND PIRANHA_CXX_FLAGS_DEBUG ${_PIRANHA_CLANG_CL_DISABLED_WARNINGS})
        list(APPEND PIRANHA_CXX_FLAGS_RELEASE ${_PIRANHA_CLANG_CL_DISABLED_WARNINGS})
        unset(_PIRANHA_CLANG_CL_DISABLED_WARNINGS)
    else()
        # Problematic MSVC cl warnings.
        list(APPEND PIRANHA_CXX_FLAGS_DEBUG "/wd4459")
        list(APPEND PIRANHA_CXX_FLAGS_RELEASE "/wd4459")
    endif()
endif()

# Find the dependencies.

# mp++.
find_package(mp++ 0.17 REQUIRED)

# abseil.
find_package(absl REQUIRED)

# Boost setup.
include(PiranhaFindBoost)

# libbacktrace.
if(PIRANHA_WITH_LIBBACKTRACE)
    set(PIRANHA_ENABLE_LIBBACKTRACE "#define PIRANHA_WITH_LIBBACKTRACE")
    find_package(libbacktrace REQUIRED)
endif()

# TBB.
find_package(TBB REQUIRED)

# Depend on DbgEng on MSVC, for the stack trace support.
if(YACMA_COMPILER_IS_MSVC)
    find_package(DbgEng REQUIRED)
endif()

# Initial setup of the piranha target.
set(PIRANHA_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cf/cf_stream_insert.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/atomic_flag_array.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/hc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/to_string.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stack_trace.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/series.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/symbols.cpp"
)

if(PIRANHA_WITH_LIBBACKTRACE)
    set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/libbacktrace_stack_trace.cpp" "${PIRANHA_SRC_FILES}")
else()
    set(PIRANHA_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/boost_stack_trace.cpp" "${PIRANHA_SRC_FILES}")
endif()

add_library(piranha SHARED "${PIRANHA_SRC_FILES}")
set_property(TARGET piranha PROPERTY VERSION "1.0")
set_property(TARGET piranha PROPERTY SOVERSION 1 )
target_compile_options(piranha PRIVATE "$<$<CONFIG:DEBUG>:${PIRANHA_CXX_FLAGS_DEBUG}>" "$<$<CONFIG:RELEASE>:${PIRANHA_CXX_FLAGS_RELEASE}>" "$<$<CONFIG:RelWithDebInfo>:${PIRANHA_CXX_FLAGS_RELEASE}>" 
                                        "$<$<CONFIG:MinSizeRel>:${PIRANHA_CXX_FLAGS_RELEASE}>")
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
target_link_libraries(piranha PUBLIC Threads::Threads mp++::mp++ absl::flat_hash_map absl::hash Boost::boost)
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

# TBB.
target_link_libraries(piranha PUBLIC TBB::tbb)

# Link to DbgEng on MSVC.
if(YACMA_COMPILER_IS_MSVC)
    target_link_libraries(piranha PUBLIC DbgEng::DbgEng)
endif()

# Configure config.hpp.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" @ONLY)

# Configure the sphinx config file.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py.in" "${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py" @ONLY)

# Installation of the header files.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/piranha" DESTINATION include)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" DESTINATION include/piranha)

# Installation of the library.
install(TARGETS piranha
    EXPORT piranha_export
    LIBRARY DESTINATION "${PIRANHA_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${PIRANHA_INSTALL_LIBDIR}"
    RUNTIME DESTINATION bin
)

if(PIRANHA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(PIRANHA_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
