if(NOT WITH_RUBY)
    return()
endif()


message("Building bindings for ruby")


find_package(Ruby REQUIRED)
include_directories(${RUBY_INCLUDE_DIRS})


function(add_ruby_module MODULE_NAME)
    set(TARGET_NAME "ruby_${MODULE_NAME}")
    set_source_files_properties(../../swig/${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
    set_property(SOURCE ../../swig/${MODULE_NAME}.i PROPERTY SWIG_MODULE_NAME ${MODULE_NAME})
    set(CMAKE_SWIG_FLAGS -module ${MODULE_NAME} -ruby)
    swig_add_library(${TARGET_NAME} LANGUAGE ruby SOURCES ../../swig/${MODULE_NAME}.i)
    set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${MODULE_NAME})
    target_compile_options(${TARGET_NAME} PUBLIC ${SWIG_COMPILE_OPTIONS})
    swig_link_libraries(${TARGET_NAME} libdnf)
    swig_link_libraries(${TARGET_NAME} ${RUBY_LIBRARIES})
endfunction()


add_ruby_module("base")
