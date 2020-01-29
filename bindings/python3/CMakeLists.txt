if(NOT WITH_PYTHON3)
    return()
endif()


message("Building bindings for python3")


find_package(Python3 REQUIRED)
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})


function(add_python3_module MODULE_NAME SWIG_MODULE_PATH)
    set(TARGET_NAME "python3_${MODULE_NAME}")
    set_source_files_properties(${SWIG_MODULE_PATH}/${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
    swig_add_library(${TARGET_NAME} LANGUAGE python SOURCES ${SWIG_MODULE_PATH}/${MODULE_NAME}.i)
    set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${MODULE_NAME})
    target_compile_options(${TARGET_NAME} PUBLIC ${SWIG_COMPILE_OPTIONS})
    swig_link_libraries(${TARGET_NAME} libdnf)
    swig_link_libraries(${TARGET_NAME} ${PYTHON_LIBRARIES})
endfunction()


add_subdirectory(libdnf)
add_subdirectory(libdnf-cli)
