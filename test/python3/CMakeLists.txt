if(NOT WITH_PYTHON3)
    return()
endif()


find_package(Python3 REQUIRED)


add_test(NAME test_python3 COMMAND ${Python3_EXECUTABLE} -m unittest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_property(TEST test_python3 PROPERTY ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/bindings/python3")
