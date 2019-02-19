# html and man documentation are separate targets, apparently there's no way to
# tell sphinx-build to do them both in one go:

if(${PYTHON_VERSION_MAJOR} STREQUAL "2")
    set(SPHINX_BUILD_NAME "sphinx-build")
else()
    set(SPHINX_BUILD_NAME "sphinx-build-3")
endif()

if(WITH_HTML)
    add_custom_target(doc-html
        PYTHONPATH=${CMAKE_BINARY_DIR}/src/python ${SPHINX_BUILD_NAME} -b html
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
        COMMENT "Building html documentation"
    )
endif()
if(WITH_MAN)
    add_custom_target(doc-man ALL
        PYTHONPATH=${CMAKE_BINARY_DIR}/src/python ${SPHINX_BUILD_NAME} -b man
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/man
        COMMENT "Building manpage documentation"
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/hawkey.3 DESTINATION share/man/man3)
endif()
