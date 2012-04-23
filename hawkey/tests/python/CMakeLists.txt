ADD_TEST(test_python /usr/bin/python "${CMAKE_CURRENT_SOURCE_DIR}/test_main.py"
		     "${CMAKE_BINARY_DIR}/src/python"
		     "${CMAKE_SOURCE_DIR}/tests/repos/")

ADD_SUBDIRECTORY (module)
