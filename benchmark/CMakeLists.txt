# NOTE: don't find a specific version as we already checked
# outside that the Boost version is appropriate.
find_package(Boost REQUIRED COMPONENTS program_options)
if(NOT TARGET Boost::program_options)
    message(STATUS "The 'Boost::program_options' imported target is missing, creating it.")
    add_library(Boost::program_options UNKNOWN IMPORTED)
    set_target_properties(Boost::program_options PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
    )
    set_target_properties(Boost::program_options PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${Boost_PROGRAM_OPTIONS_LIBRARY}"
    )
endif()

add_library(obake_benchmark STATIC sparse_dense_options.cpp)
target_link_libraries(obake_benchmark PRIVATE Boost::program_options)
target_compile_options(obake_benchmark PRIVATE
  "$<$<CONFIG:Debug>:${OBAKE_CXX_FLAGS_DEBUG}>"
  "$<$<CONFIG:Release>:${OBAKE_CXX_FLAGS_RELEASE}>"
  "$<$<CONFIG:RelWithDebInfo>:${OBAKE_CXX_FLAGS_RELEASE}>"
  "$<$<CONFIG:MinSizeRel>:${OBAKE_CXX_FLAGS_RELEASE}>"
)
if(NOT CMAKE_CXX_STANDARD)
  # The user did not provide the CMAKE_CXX_STANDARD variable,
  # go with the default (C++17).
  set_property(TARGET obake_benchmark PROPERTY CXX_STANDARD 17)
endif()
set_property(TARGET obake_benchmark PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET obake_benchmark PROPERTY CXX_EXTENSIONS NO)

function(ADD_OBAKE_BENCHMARK arg1)
  add_executable(${arg1} ${arg1}.cpp)
  target_link_libraries(${arg1} PRIVATE obake obake_benchmark)
  target_compile_options(${arg1} PRIVATE
    "$<$<CONFIG:Debug>:${OBAKE_CXX_FLAGS_DEBUG}>"
    "$<$<CONFIG:Release>:${OBAKE_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:RelWithDebInfo>:${OBAKE_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:MinSizeRel>:${OBAKE_CXX_FLAGS_RELEASE}>"
  )
  if(NOT CMAKE_CXX_STANDARD)
    # The user did not provide the CMAKE_CXX_STANDARD variable,
    # go with the default (C++17).
    set_property(TARGET ${arg1} PROPERTY CXX_STANDARD 17)
  endif()
  set_property(TARGET ${arg1} PROPERTY CXX_STANDARD_REQUIRED YES)
  set_property(TARGET ${arg1} PROPERTY CXX_EXTENSIONS NO)
endfunction()

ADD_OBAKE_BENCHMARK(audi_01)
ADD_OBAKE_BENCHMARK(dense_4_vars)
ADD_OBAKE_BENCHMARK(dense_02)
ADD_OBAKE_BENCHMARK(rectangular_01)
ADD_OBAKE_BENCHMARK(sparse)
ADD_OBAKE_BENCHMARK(sparse_02_truncated)
