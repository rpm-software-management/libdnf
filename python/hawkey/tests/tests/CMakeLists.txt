execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write('%s.%s' % (sys.version_info.major, sys.version_info.minor))" OUTPUT_VARIABLE PYTHON_MAJOR_DOT_MINOR_VERSION)
execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "create_symlink" "${CMAKE_BINARY_DIR}/bindings/python" "${CMAKE_CURRENT_BINARY_DIR}/libdnf")
configure_file("run_nosetests.in"  "run_nosetests")
add_test(test_python run_nosetests -s ${CMAKE_CURRENT_SOURCE_DIR})
