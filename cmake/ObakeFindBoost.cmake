# NOTE: stacktrace available since 1.65.0.
set(_OBAKE_BOOST_MINIMUM_VERSION 1.65.0)
find_package(Boost ${_OBAKE_BOOST_MINIMUM_VERSION} REQUIRED COMPONENTS serialization)

message(STATUS "Detected Boost version: ${Boost_VERSION}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

# Might need to recreate targets if they are missing (e.g., older CMake versions).
if(NOT TARGET Boost::boost)
    message(STATUS "The 'Boost::boost' target is missing, creating it.")
    add_library(Boost::boost INTERFACE IMPORTED)
    set_target_properties(Boost::boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
endif()

if(NOT TARGET Boost::serialization)
    message(STATUS "The 'Boost::serialization' imported target is missing, creating it.")
    add_library(Boost::serialization UNKNOWN IMPORTED)
    set_target_properties(Boost::serialization PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
    )
    set_target_properties(Boost::serialization PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${Boost_SERIALIZATION_LIBRARY}"
    )
endif()

unset(_OBAKE_BOOST_MINIMUM_VERSION)
