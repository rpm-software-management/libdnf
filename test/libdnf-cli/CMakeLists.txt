if(NOT WITH_LIBDNF_CLI)
    return()
endif()


pkg_check_modules(CPPUNIT REQUIRED cppunit)


set(TEST_LIBDNF_CLI_SOURCES
    # share the test runner code with libdnf
    ${CMAKE_CURRENT_SOURCE_DIR}/../libdnf/run_tests.cpp
)


#add_subdirectory(...)


add_executable(run_tests_cli ${TEST_LIBDNF_CLI_SOURCES})
target_link_directories(run_tests_cli PUBLIC ${CMAKE_BINARY_DIR}/libdnf)
target_link_libraries(run_tests_cli stdc++ libdnf cppunit)


add_test(NAME test_libdnf_cli COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests_cli DEPENDS run_tests_cli COMMENT "Running libdnf-cli C++ tests...")
set_property(TEST test_libdnf_cli PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libdnf:${CMAKE_BINARY_DIR}/libdnf-cli")
