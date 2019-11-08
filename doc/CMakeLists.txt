configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py)

add_custom_target(doxygen
    doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Building doxygen xml files"
)
add_custom_target(doc-html
    sphinx-build -c ${CMAKE_CURRENT_BINARY_DIR} -b html
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/_build
    COMMENT "Building html documentation"
)
add_custom_target(doc)
add_dependencies(doc-html doxygen)
add_dependencies(doc doc-html)
