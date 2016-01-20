EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write('%s.%s' % (sys.version_info.major, sys.version_info.minor))" OUTPUT_VARIABLE PYTHON_MAJOR_DOT_MINOR_VERSION)
CONFIGURE_FILE("run_nosetests.in"  "run_nosetests")
ADD_TEST(test_python run_nosetests -s ${CMAKE_CURRENT_SOURCE_DIR})
