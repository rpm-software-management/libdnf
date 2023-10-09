find_package(PythonInstDir)
find_package(SWIG REQUIRED)
include(UseSWIG)

message(STATUS "Building for python${PYTHON_VERSION_MAJOR}")

find_package(PythonInstDir)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${PYTHON_INCLUDE_PATH})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # using 64bit architecture
    set(CMAKE_SWIG_FLAGS "-DSWIGWORDSIZE64")
endif()

# The libdnf API is under development now. This enables it for internal usage.
set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} "-DLIBDNF_UNSTABLE_API")

function(libdnf_swig_module name)
    set(source ../swig/${name}.i)
    set_source_files_properties(${source} PROPERTIES CPLUSPLUS ON)
    swig_add_module(${name} python ${source})

    foreach(lib ${ARGN})
        swig_link_libraries(${name} ${lib})
    endforeach()

    install(TARGETS _${name} LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
    install(FILES ${CMAKE_BINARY_DIR}/bindings/python/${name}.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
endfunction()

libdnf_swig_module(error libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(common_types libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(conf libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(module libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(repo libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(smartcols libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(transaction libdnf ${PYTHON_LIBRARY})
libdnf_swig_module(utils libdnf ${PYTHON_LIBRARY} ${SQLite3_LIBRARIES})

configure_file(__init__.py ${CMAKE_CURRENT_BINARY_DIR}/__init__.py COPYONLY)
install(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set(DISTINFO_PATH "${CMAKE_CURRENT_BINARY_DIR}/libdnf-${LIBDNF_VERSION}.dist-info")
set(METADATA_FILE "${DISTINFO_PATH}/METADATA")
file(MAKE_DIRECTORY ${DISTINFO_PATH})
file(WRITE ${METADATA_FILE} "Metadata-Version: 2.1\n")
file(APPEND ${METADATA_FILE} "Name: libdnf\n")
file(APPEND ${METADATA_FILE} "Version: ${LIBDNF_VERSION}\n")
install(DIRECTORY ${DISTINFO_PATH} DESTINATION ${PYTHON_INSTALL_DIR})
