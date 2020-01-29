if(NOT WITH_LIBDNF_CLI)
    return()
endif()


message("Building bindings for ruby: libdnf-cli")


foreach(MODULE_NAME ${SWIG_LIBDNF_CLI_MODULES})
    add_ruby_module(${MODULE_NAME} ../../libdnf-cli)
endforeach()
