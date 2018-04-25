OPTION(DISABLE_VALGRIND "Disables valgrind tests for hawkey and libdnf" OFF)

ADD_SUBDIRECTORY(libdnf/swdb)
ADD_SUBDIRECTORY(libdnf/repo)
ADD_SUBDIRECTORY (hawkey)
ADD_SUBDIRECTORY (libdnf)



pkg_check_modules(CPPUNIT REQUIRED cppunit)

SET (LIBDNF_TEST_SOURCES
        ${LIBDNF_TEST_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.cpp)

ADD_EXECUTABLE(run_tests ${LIBDNF_TEST_SOURCES} ${LIBDNF_TEST_HEADERS})
TARGET_LINK_LIBRARIES(run_tests libdnf cppunit)

ADD_TEST(NAME test_cpp COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests DEPENDS run_tests COMMENT "Running CPPUNIT tests...")