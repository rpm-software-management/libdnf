if(NOT WITH_DNFDAEMON_SERVER)
    return()
endif()


pkg_check_modules(CPPUNIT REQUIRED cppunit)


set(TEST_DNFDAEMON_SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.cpp
)


#add_subdirectory(...)


add_executable(run_tests_dnfdaemon_server ${TEST_DNFDAEMON_SERVER_SOURCES})
target_link_directories(run_tests_dnfdaemon_server PUBLIC ${CMAKE_BINARY_DIR}/libdnf ${CMAKE_BINARY_DIR}/libdnf-cli)
target_link_libraries(run_tests_dnfdaemon_server stdc++ libdnf libdnf-cli cppunit)


add_test(NAME test_dnfdaemon_server COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests_dnfdaemon_server DEPENDS run_tests_dnfdaemon_server COMMENT "Running dnfdaemon-server tests...")
