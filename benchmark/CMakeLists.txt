function(ADD_OBAKE_BENCHMARK arg1)
  add_executable(${arg1} ${arg1}.cpp)
  target_link_libraries(${arg1} PRIVATE obake)
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
ADD_OBAKE_BENCHMARK(dense_01)
ADD_OBAKE_BENCHMARK(dense_02)
ADD_OBAKE_BENCHMARK(rectangular_01)
ADD_OBAKE_BENCHMARK(sparse_01)
ADD_OBAKE_BENCHMARK(sparse_02)
ADD_OBAKE_BENCHMARK(sparse_02_truncated)
ADD_OBAKE_BENCHMARK(sparse_03)
ADD_OBAKE_BENCHMARK(sparse_04)
