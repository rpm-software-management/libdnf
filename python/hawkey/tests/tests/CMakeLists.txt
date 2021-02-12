execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "create_symlink" "${CMAKE_BINARY_DIR}/bindings/python" "${CMAKE_CURRENT_BINARY_DIR}/libdnf")
add_test(NAME test_python3_libdnf COMMAND ${PYTHON_EXECUTABLE} -m unittest discover -t "${CMAKE_CURRENT_SOURCE_DIR}/.." -s "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(TEST test_python3_libdnf PROPERTY ENVIRONMENT
    "PYTHONPATH=${CMAKE_BINARY_DIR}/python/hawkey/"
    "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libdnf/"
)
