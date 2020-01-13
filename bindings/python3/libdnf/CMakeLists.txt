message("Building bindings for python3")


find_package(Python3 REQUIRED)
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})


function(add_python3_module MODULE_NAME)
    set(TARGET_NAME "python3_${MODULE_NAME}")
    set_source_files_properties(../../swig/${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
    swig_add_library(${TARGET_NAME} LANGUAGE python SOURCES ../../swig/${MODULE_NAME}.i)
    set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${MODULE_NAME})
    target_compile_options(${TARGET_NAME} PUBLIC ${SWIG_COMPILE_OPTIONS})
    swig_link_libraries(${TARGET_NAME} libdnf)
    swig_link_libraries(${TARGET_NAME} ${PYTHON_LIBRARIES})
endfunction()


add_python3_module("base")
add_python3_module("utils")


configure_file(__init__.py ${CMAKE_CURRENT_BINARY_DIR}/__init__.py COPYONLY)
install(FILES __init__.py DESTINATION ${Python3_SITEARCH}/libdnf)
