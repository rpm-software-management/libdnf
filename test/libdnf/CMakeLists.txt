pkg_check_modules(CPPUNIT REQUIRED cppunit)


set(TEST_LIBDNF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.cpp
)


add_subdirectory(utils)


add_executable(run_tests ${TEST_LIBDNF_SOURCES})
target_link_directories(run_tests PUBLIC ${CMAKE_BINARY_DIR}/libdnf)
target_link_libraries(run_tests stdc++ libdnf cppunit)


add_test(NAME test_libdnf COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests DEPENDS run_tests COMMENT "Running libdnf C++ tests...")
set_property(TEST test_libdnf PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libdnf")
