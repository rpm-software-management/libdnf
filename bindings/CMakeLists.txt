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
#add_compile_options(-Wno-cast-align)
#add_compile_options(-Wno-deprecated-declarations)
#add_compile_options(-Wno-missing-declarations)
#add_compile_options(-Wno-missing-field-initializers)
#add_compile_options(-Wno-unused-function)
#add_compile_options(-Wno-unused-parameter)


if(WITH_GO)
    add_subdirectory(go/libdnf)
endif()


if(WITH_PERL5)
    add_subdirectory(perl5/libdnf)
endif()


if(WITH_PYTHON3)
    add_subdirectory(python3/libdnf)
endif()


if(WITH_RUBY)
    add_subdirectory(ruby/libdnf)
endif()
