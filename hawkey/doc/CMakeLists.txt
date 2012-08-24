ADD_CUSTOM_TARGET (doc
		  sphinx-build -b html -b man ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
		  COMMENT "Building documentation")

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/hawkey.1 DESTINATION share/man/man1)
