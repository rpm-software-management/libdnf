ADD_SUBDIRECTORY(module)
ADD_SUBDIRECTORY(repo)
ADD_SUBDIRECTORY(transaction)

SET (LIBDNF_TEST_SOURCES
        ${LIBDNF_TEST_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/TestSack.cpp
        PARENT_SCOPE
        )

ADD_EXECUTABLE(test_libdnf_main dnf-self-test.c)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/tests/modules/yum.repos.d/test.repo.in ${CMAKE_SOURCE_DIR}/data/tests/modules/yum.repos.d/test.repo @ONLY)

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
