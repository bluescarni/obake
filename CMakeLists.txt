# NOTE: C++17 supported since CMake 3.8.0:
# https://cmake.org/cmake/help/v3.8/prop_tgt/CXX_STANDARD.html
cmake_minimum_required(VERSION 3.8.0)

# Set default build type to "Release".
# NOTE: this should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif()

# NOTE: obake is C++, however we are linking to C
# libraries (e.g., libbacktrace). Moreover, if we don't
# enable C here, on some configurations (e.g., Ubuntu
# + clang) the threading detection/setup seems to break
# down.
project(obake VERSION 0.4.0 LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/yacma")

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "obake version: ${obake_VERSION}")

# The build options.
option(OBAKE_BUILD_TESTS "Build unit tests." OFF)
option(OBAKE_BUILD_BENCHMARKS "Build benchmarks." OFF)
option(OBAKE_WITH_LIBBACKTRACE "Use libbacktrace for improved stack traces." OFF)

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
    set(_OBAKE_INSTALL_LIBDIR_DEFAULT "${CMAKE_INSTALL_LIBDIR}")
else()
    set(_OBAKE_INSTALL_LIBDIR_DEFAULT "lib")
endif()
if(NOT OBAKE_INSTALL_LIBDIR)
    set(OBAKE_INSTALL_LIBDIR "${_OBAKE_INSTALL_LIBDIR_DEFAULT}" CACHE STRING
        "Library installation directory." FORCE)
endif()
mark_as_advanced(OBAKE_INSTALL_LIBDIR)
message(STATUS "Library installation directory: ${OBAKE_INSTALL_LIBDIR}")

# Assemble the flags.
set(OBAKE_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG})
set(OBAKE_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS})

if(YACMA_COMPILER_IS_MSVC)
    include(CheckCXXCompilerFlag)
    # Disable the idiotic minmax macros on MSVC (both cl and clang-cl).
    # Also, enable the bigobj flag and the WIN32_LEAN_AND_MEAN definitions:
    # https://stackoverflow.com/questions/11040133/what-does-defining-win32-lean-and-mean-exclude-exactly
    list(APPEND OBAKE_CXX_FLAGS_DEBUG "-DNOMINMAX" "/bigobj" "-DWIN32_LEAN_AND_MEAN")
    list(APPEND OBAKE_CXX_FLAGS_RELEASE "-DNOMINMAX" "/bigobj" "-DWIN32_LEAN_AND_MEAN")
    # Enable strict conformance mode, if supported.
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_cxx_compiler_flag("/permissive-" _OBAKE_MSVC_SUPPORTS_STRICT_CONFORMANCE)
    unset(CMAKE_REQUIRED_QUIET)
    if(_OBAKE_MSVC_SUPPORTS_STRICT_CONFORMANCE)
        message(STATUS "The '/permissive-' flag is supported, enabling it.")
        list(APPEND OBAKE_CXX_FLAGS_DEBUG "/permissive-")
        list(APPEND OBAKE_CXX_FLAGS_RELEASE "/permissive-")
    endif()
    unset(_OBAKE_MSVC_SUPPORTS_STRICT_CONFORMANCE)
    if(YACMA_COMPILER_IS_CLANGXX)
        # clang-cl emits various warnings from GMP/MPFR, let's just silence them.
        # NOTE: at one point in the recent past, MSVC added an options similar to GCC's isystem:
        # https://blogs.msdn.microsoft.com/vcblog/2017/12/13/broken-warnings-theory/
        # We probably just need to wait for this to be picked up by CMake/clang-cl. Let's
        # revisit the issue in the future.
        list(APPEND _OBAKE_CLANG_CL_DISABLED_WARNINGS
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
        list(APPEND OBAKE_CXX_FLAGS_DEBUG ${_OBAKE_CLANG_CL_DISABLED_WARNINGS})
        list(APPEND OBAKE_CXX_FLAGS_RELEASE ${_OBAKE_CLANG_CL_DISABLED_WARNINGS})
        unset(_OBAKE_CLANG_CL_DISABLED_WARNINGS)
    else()
        # Problematic MSVC cl warnings.
        list(APPEND OBAKE_CXX_FLAGS_DEBUG "/wd4459")
        list(APPEND OBAKE_CXX_FLAGS_RELEASE "/wd4459")
    endif()
endif()
if(MINGW)
    # In MinGW some tests generate big object files.
    message(STATUS "Enabling the '-Wa,-mbig-obj' flag for MinGW.")
    list(APPEND OBAKE_CXX_FLAGS_DEBUG "-Wa,-mbig-obj")
    list(APPEND OBAKE_CXX_FLAGS_RELEASE "-Wa,-mbig-obj")
endif()

# Find the dependencies.

# mp++.
# NOTE: put the minimum version in a variable
# so that we can re-use it in the config-file package
# machinery below.
set (_OBAKE_MIN_MPPP_VERSION 0.17)
find_package(mp++ REQUIRED)
if(${mp++_VERSION} VERSION_LESS ${_OBAKE_MIN_MPPP_VERSION})
    message(FATAL_ERROR "The minimum mp++ version required by obake is ${_OBAKE_MIN_MPPP_VERSION}, but version ${mp++_VERSION} was found instead.")
endif()

# abseil.
find_package(absl REQUIRED)

# Boost setup.
include(ObakeFindBoost)

# libbacktrace.
if(OBAKE_WITH_LIBBACKTRACE)
    set(OBAKE_ENABLE_LIBBACKTRACE "#define OBAKE_WITH_LIBBACKTRACE")
    find_package(libbacktrace REQUIRED)
endif()

# TBB.
find_package(TBB REQUIRED)

# Depend on DbgEng on WIN32, for the stack trace support.
if(WIN32)
    find_package(DbgEng REQUIRED)
endif()

# Wrap the CMAKE_DL_LIBS variable in an imported target.
include(ObakeFindDl)

# Initial setup of the obake target.
set(OBAKE_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cf/cf_stream_insert.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/atomic_flag_array.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/hc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/detail/to_string.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stack_trace.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/series.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/symbols.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tex_stream_insert.cpp"
)

if(OBAKE_WITH_LIBBACKTRACE)
    set(OBAKE_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/libbacktrace_stack_trace.cpp" "${OBAKE_SRC_FILES}")
else()
    set(OBAKE_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/boost_stack_trace.cpp" "${OBAKE_SRC_FILES}")
endif()

add_library(obake SHARED "${OBAKE_SRC_FILES}")
set_property(TARGET obake PROPERTY VERSION "4.0")
set_property(TARGET obake PROPERTY SOVERSION 4)
target_compile_options(obake PRIVATE
    "$<$<CONFIG:Debug>:${OBAKE_CXX_FLAGS_DEBUG}>"
    "$<$<CONFIG:Release>:${OBAKE_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:RelWithDebInfo>:${OBAKE_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:MinSizeRel>:${OBAKE_CXX_FLAGS_RELEASE}>"
)
set_target_properties(obake PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(obake PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
if(NOT CMAKE_CXX_STANDARD)
    # The user did not provide the CMAKE_CXX_STANDARD variable,
    # go with the default (C++17).
    message(STATUS "Setting the C++ standard version to the default value (17).")
    set_property(TARGET obake PROPERTY CXX_STANDARD 17)
else()
    message(STATUS "Using the manually-specified value for the C++ standard version (${CMAKE_CXX_STANDARD}).")
endif()
set_property(TARGET obake PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET obake PROPERTY CXX_EXTENSIONS NO)
# Add the include dirs to the obake target's interface.
target_include_directories(obake PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Link the deps.
target_link_libraries(obake PUBLIC
    Threads::Threads
    mp++::mp++
    absl::flat_hash_map
    absl::flat_hash_set
    absl::hash
    Boost::boost
    Boost::serialization
    Boost::disable_autolinking
    TBB::tbb
)
if(OBAKE_WITH_LIBBACKTRACE)
    target_link_libraries(obake PRIVATE libbacktrace::libbacktrace)
else()
    # NOTE: Boost stacktrace sometimes requires linking to the system
    # DL libs.
    target_link_libraries(obake PRIVATE obake::dl)
endif()

# Link to DbgEng on Windows.
if(WIN32)
    target_link_libraries(obake PRIVATE DbgEng::DbgEng)
endif()

# Configure config.hpp.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/obake/config.hpp" @ONLY)

# Configure the sphinx config file.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py.in" "${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py" @ONLY)

# Installation of the header files.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/obake" DESTINATION include)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/obake/config.hpp" DESTINATION include/obake)

# Installation of the library.
install(TARGETS obake
    EXPORT obake_export
    LIBRARY DESTINATION "${OBAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${OBAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION bin
)

# Setup of the optional deps.
set(_OBAKE_CONFIG_OPTIONAL_DEPS)
if(OBAKE_WITH_LIBBACKTRACE)
    set(_OBAKE_CONFIG_OPTIONAL_DEPS "${_OBAKE_CONFIG_OPTIONAL_DEPS}find_package(libbacktrace REQUIRED)\n")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/obake-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/obake-config.cmake" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/obake-config.cmake" DESTINATION "${OBAKE_INSTALL_LIBDIR}/cmake/obake")
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindDbgEng.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindTBB.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findlibbacktrace.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ObakeFindBoost.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ObakeFindDl.cmake"
DESTINATION "${OBAKE_INSTALL_LIBDIR}/cmake/obake")
install(EXPORT obake_export NAMESPACE obake:: DESTINATION "${OBAKE_INSTALL_LIBDIR}/cmake/obake")
# Take care of versioning.
include(CMakePackageConfigHelpers)
# NOTE: SameMinorVersion available only
# since CMake 3.11.
if(${CMAKE_VERSION} VERSION_LESS "3.11.0")
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/obake-config-version.cmake" COMPATIBILITY SameMajorVersion)
else()
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/obake-config-version.cmake" COMPATIBILITY SameMinorVersion)
endif()
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/obake-config-version.cmake" DESTINATION "${OBAKE_INSTALL_LIBDIR}/cmake/obake")

if(OBAKE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(OBAKE_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
