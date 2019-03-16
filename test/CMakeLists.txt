# We have some multithreaded tests, so let's activate threading support.
include(YACMAThreadingSetup)

function(ADD_PIRANHA_TESTCASE arg1)
  add_executable(${arg1} ${arg1}.cpp)
  target_link_libraries(${arg1} piranha Threads::Threads)
  target_compile_options(${arg1} PRIVATE "$<$<CONFIG:DEBUG>:${PIRANHA_CXX_FLAGS_DEBUG}>" "$<$<CONFIG:RELEASE>:${PIRANHA_CXX_FLAGS_RELEASE}>")
  set_target_properties(${arg1} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  set_target_properties(${arg1} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
  set_property(TARGET ${arg1} PROPERTY CXX_STANDARD 17)
  set_property(TARGET ${arg1} PROPERTY CXX_STANDARD_REQUIRED YES)
  set_property(TARGET ${arg1} PROPERTY CXX_EXTENSIONS NO)
  add_test(${arg1} ${arg1})
endfunction()

ADD_PIRANHA_TESTCASE(math_is_zero)
ADD_PIRANHA_TESTCASE(math_pow)
ADD_PIRANHA_TESTCASE(math_safe_convert)
ADD_PIRANHA_TESTCASE(polynomials_polynomial)
ADD_PIRANHA_TESTCASE(series)
ADD_PIRANHA_TESTCASE(type_traits)
ADD_PIRANHA_TESTCASE(utils_demangle)

