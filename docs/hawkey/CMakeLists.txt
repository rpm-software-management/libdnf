# html and man documentation are separate targets, apparently there's no way to
# tell sphinx-build to do them both in one go:

if (${PYTHON_DESIRED} STREQUAL "2")
    SET(SPHINX_BUILD_NAME "sphinx-build")
else()
    SET(SPHINX_BUILD_NAME "sphinx-build-3")
endif()

ADD_CUSTOM_TARGET (doc-html ALL
		  PYTHONPATH=${CMAKE_BINARY_DIR}/src/python ${SPHINX_BUILD_NAME} -b html
		  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/html
		  COMMENT "Building html documentation")
ADD_CUSTOM_TARGET (doc-man ALL
		  PYTHONPATH=${CMAKE_BINARY_DIR}/src/python ${SPHINX_BUILD_NAME} -b man
		  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/man
		  COMMENT "Building manpage documentation")
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/hawkey.3 DESTINATION share/man/man3)
