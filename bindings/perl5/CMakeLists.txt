if(NOT WITH_PERL5)
    return()
endif()


message("Building bindings for perl5")


find_package(Perl REQUIRED)
find_package(PerlLibs REQUIRED)
include_directories(${PERL_INCLUDE_PATH})


function(add_perl5_module MODULE_NAME SWIG_MODULE_PATH)
    set(TARGET_NAME "perl5_${MODULE_NAME}")
    set_source_files_properties(${SWIG_MODULE_PATH}/${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
    set_property(SOURCE ${SWIG_MODULE_PATH}/${MODULE_NAME}.i PROPERTY SWIG_MODULE_NAME libdnf::${MODULE_NAME})
    swig_add_library(${TARGET_NAME} LANGUAGE perl SOURCES ${SWIG_MODULE_PATH}/${MODULE_NAME}.i)
    set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${MODULE_NAME})
    target_compile_options(${TARGET_NAME} PUBLIC ${SWIG_COMPILE_OPTIONS})
    swig_link_libraries(${TARGET_NAME} libdnf)
    swig_link_libraries(${TARGET_NAME} ${PERL_LIBRARY})
endfunction()


add_subdirectory(libdnf)
add_subdirectory(libdnf-cli)
