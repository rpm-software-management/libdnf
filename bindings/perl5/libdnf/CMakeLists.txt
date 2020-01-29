message("Building bindings for perl5: libdnf")


foreach(MODULE_NAME ${SWIG_LIBDNF_MODULES})
    add_perl5_module(${MODULE_NAME} ../../libdnf)
endforeach()
