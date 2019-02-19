option(DISABLE_VALGRIND "Disables valgrind tests for hawkey and libdnf" OFF)

add_subdirectory(libdnf/module)
add_subdirectory(libdnf/repo)
add_subdirectory(libdnf/transaction)
add_subdirectory(hawkey)
add_subdirectory(libdnf)



pkg_check_modules(CPPUNIT REQUIRED cppunit)

set(LIBDNF_TEST_SOURCES
    ${LIBDNF_TEST_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.cpp
)

add_executable(run_tests ${LIBDNF_TEST_SOURCES} ${LIBDNF_TEST_HEADERS})
target_link_libraries(run_tests libdnf cppunit)

add_test(NAME test_cpp COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests DEPENDS run_tests COMMENT "Running CPPUNIT tests...")
