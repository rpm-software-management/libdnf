if(NOT WITH_GO)
    return()
endif()


message("Building bindings for go")


function(add_go_module MODULE_NAME SWIG_MODULE_PATH)
    set(TARGET_NAME "go_${MODULE_NAME}")
    set_source_files_properties(${SWIG_MODULE_PATH}/${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
    set(CMAKE_SWIG_FLAGS -module ${TARGET_NAME} -cgo -intgosize 64)
    swig_add_library(${TARGET_NAME} LANGUAGE go SOURCES ${SWIG_MODULE_PATH}/${MODULE_NAME}.i)
    set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${MODULE_NAME})
    target_compile_options(${TARGET_NAME} PUBLIC ${SWIG_COMPILE_OPTIONS})
    swig_link_libraries(${TARGET_NAME} libdnf)
endfunction()


add_subdirectory(libdnf)
add_subdirectory(libdnf-cli)
