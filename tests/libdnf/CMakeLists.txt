ADD_EXECUTABLE(test_libdnf_main dnf-self-test.c)

TARGET_LINK_LIBRARIES(test_libdnf_main
                      libdnf
                      ${REPO_LIBRARIES}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GIO_UNIX_LIBRARIES}
                      ${SOLV_LIBRARY}
                      ${SOLVEXT_LIBRARY}
                      ${RPMDB_LIBRARY})

ADD_TEST(test_libdnf_main test_libdnf_main)
IF (NOT DISABLE_VALGRIND AND VALGRIND_PROGRAM)
    ADD_TEST(test_libdnf_valgrind ${VALGRIND_PROGRAM} --error-exitcode=1 --leak-check=full
             --suppressions=${CMAKE_SOURCE_DIR}/tests/glib.supp
             --suppressions=${CMAKE_SOURCE_DIR}/tests/nss.supp
             --suppressions=${CMAKE_SOURCE_DIR}/tests/giovfs.supp
             --suppressions=${CMAKE_SOURCE_DIR}/tests/librepo.supp
             ${CMAKE_CURRENT_BINARY_DIR}/test_libdnf_main)
ENDIF()


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

FIND_PACKAGE(PkgConfig REQUIRED)
pkg_check_modules(CPPUNIT cppunit)
INCLUDE_DIRECTORIES(${CPPUNIT_INCLUDE_DIR})

SET (LIBDNF_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.cpp
)

INCLUDE_DIRECTORIES(swdb)
ADD_SUBDIRECTORY("swdb")

ADD_EXECUTABLE(run_tests ${LIBDNF_TEST_SOURCES} ${LIBDNF_TEST_HEADERS})
SET_TARGET_PROPERTIES(run_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${COMMON_RUNTIME_OUTPUT_DIRECTORY}")
TARGET_LINK_LIBRARIES(run_tests libdnf cppunit)

ADD_TEST(NAME test_cpp COMMAND run_tests DEPENDS run_tests COMMENT "Running CPPUNIT tests...")
