message("Building bindings for go: libdnf")


foreach(MODULE_NAME ${SWIG_LIBDNF_MODULES})
    add_go_module(${MODULE_NAME} ../../libdnf)
endforeach()
