if(NOT WITH_LIBDNF_CLI)
    return()
endif()


message("Building bindings for go: libdnf-cli")


foreach(MODULE_NAME ${SWIG_LIBDNF_CLI_MODULES})
    add_go_module(${MODULE_NAME} ../../libdnf-cli)
endforeach()
