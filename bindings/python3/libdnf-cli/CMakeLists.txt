if(NOT WITH_LIBDNF_CLI)
    return()
endif()


message("Building bindings for python3: libdnf-cli")


file(REMOVE __init__.py)
foreach(MODULE_NAME ${SWIG_LIBDNF_CLI_MODULES})
    add_python3_module(${MODULE_NAME} ../../libdnf-cli)
    file(APPEND __init__.py "from . import ${MODULE_NAME}\n")
endforeach()


configure_file(__init__.py ${CMAKE_CURRENT_BINARY_DIR}/__init__.py COPYONLY)
install(FILES __init__.py DESTINATION ${Python3_SITEARCH}/libdnf-cli)
