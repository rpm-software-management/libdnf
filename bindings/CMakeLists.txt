if(NOT (WITH_GO OR WITH_PERL5 OR WITH_PYTHON3 OR WITH_RUBY))
    return()
endif()


message("Building bindings")


find_package(SWIG REQUIRED)
include(UseSWIG)


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # using 64bit architecture
    set(CMAKE_SWIG_FLAGS "-DSWIGWORDSIZE64")
endif()


include_directories(${CMAKE_SOURCE_DIR}/include)


# SWIG is flooding logs with warnings we can't fix -> turn them off
set(SWIG_COMPILE_OPTIONS
    -Wno-cast-align
    -Wno-conversion
    -Wno-deprecated-declarations
    -Wno-missing-declarations
    -Wno-missing-field-initializers
    -Wno-unused-function
    -Wno-unused-parameter
)


add_subdirectory(go/libdnf)
add_subdirectory(perl5/libdnf)
add_subdirectory(python3/libdnf)
add_subdirectory(ruby/libdnf)
