# html and man documentation are separate targets, apparently there's no way to
# tell sphinx-build to do them both in one go:


find_program(SPHINX_PROGRAM NAMES "sphinx-build-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
if(NOT EXISTS ${SPHINX_PROGRAM})
    find_program(SPHINX_PROGRAM NAMES sphinx-build-${PYTHON_VERSION_MAJOR})
endif()
if(NOT EXISTS ${SPHINX_PROGRAM})
    find_program(SPHINX_PROGRAM NAMES sphinx-build)
endif()
if(NOT EXISTS ${SPHINX_PROGRAM})
    message(FATAL_ERROR "Sphinx program not found." )
endif()

if(WITH_HTML)
    add_custom_target(doc-html
        PYTHONPATH=${CMAKE_BINARY_DIR}/src/python ${SPHINX_PROGRAM} -b html
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
        COMMENT "Building html documentation"
    )
endif()
if(WITH_MAN)
    add_custom_target(doc-man ALL
        PYTHONPATH=${CMAKE_BINARY_DIR}/src/python ${SPHINX_PROGRAM} -b man
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/man
        COMMENT "Building manpage documentation"
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/hawkey.3 DESTINATION share/man/man3)
endif()
