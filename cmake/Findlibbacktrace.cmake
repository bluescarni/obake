# Copyright (c) 2016-2020 Francesco Biscani, <bluescarni@gmail.com>

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ------------------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

if(libbacktrace_INCLUDE_DIR AND libbacktrace_LIBRARY)
	# Already in cache, be silent
	set(libbacktrace_FIND_QUIETLY TRUE)
endif()

find_path(libbacktrace_INCLUDE_DIR NAMES backtrace.h)
find_library(libbacktrace_LIBRARY NAMES backtrace)

if(NOT libbacktrace_INCLUDE_DIR OR NOT libbacktrace_LIBRARY)
    cmake_push_check_state(RESET)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "backtrace")
    CHECK_CXX_SOURCE_COMPILES("
        #include <backtrace.h>
        int main(void){
            auto bt_state = ::backtrace_create_state(nullptr, 0, nullptr, nullptr);
        }"
	libbacktrace_USE_DIRECTLY)
    cmake_pop_check_state()
    if (libbacktrace_USE_DIRECTLY)
        set(libbacktrace_INCLUDE_DIR "unused" CACHE PATH "" FORCE)
        set(libbacktrace_LIBRARY "backtrace" CACHE FILEPATH "" FORCE)
    endif()
endif()

find_package_handle_standard_args(libbacktrace DEFAULT_MSG libbacktrace_LIBRARY libbacktrace_INCLUDE_DIR)

mark_as_advanced(libbacktrace_INCLUDE_DIR libbacktrace_LIBRARY)

# NOTE: this has been adapted from CMake's FindPNG.cmake.
if(libbacktrace_FOUND AND NOT TARGET libbacktrace::libbacktrace)
	message(STATUS "Creating the 'libbacktrace::libbacktrace' imported target.")
	if(libbacktrace_USE_DIRECTLY)
		message(STATUS "libbacktrace will be included and linked directly.")
		# If we are using it directly, we must define an interface library,
		# as we do not have the full path to the shared library.
		add_library(libbacktrace::libbacktrace INTERFACE IMPORTED)
		set_target_properties(libbacktrace::libbacktrace PROPERTIES INTERFACE_LINK_LIBRARIES "${libbacktrace_LIBRARY}")
	else()
		# Otherwise, we proceed as usual.
		add_library(libbacktrace::libbacktrace UNKNOWN IMPORTED)
		set_target_properties(libbacktrace::libbacktrace PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${libbacktrace_INCLUDE_DIR}"
			IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${libbacktrace_LIBRARY}")
	endif()
endif()
