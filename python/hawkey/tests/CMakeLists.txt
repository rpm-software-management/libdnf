ADD_SUBDIRECTORY (module)

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write('%s.%s' % (sys.version_info.major, sys.version_info.minor))" OUTPUT_VARIABLE PYTHON_MAJOR_DOT_MINOR_VERSION)

ADD_TEST(test_hawkey_python nosetests-${PYTHON_MAJOR_DOT_MINOR_VERSION} -s ${CMAKE_CURRENT_SOURCE_DIR}/tests)
