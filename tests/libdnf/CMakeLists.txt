add_executable(test_libdnf_main dnf-self-test.c)

configure_file(${CMAKE_SOURCE_DIR}/data/tests/modules/yum.repos.d/test.repo.in ${CMAKE_SOURCE_DIR}/data/tests/modules/yum.repos.d/test.repo @ONLY)

target_link_libraries(test_libdnf_main
    libdnf
    ${REPO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GIO_UNIX_LIBRARIES}
    ${SOLV_LIBRARY}
    ${SOLVEXT_LIBRARY}
    ${RPMDB_LIBRARY}
)

add_test(test_libdnf_main test_libdnf_main)
if(NOT DISABLE_VALGRIND AND VALGRIND_PROGRAM)
    add_test(test_libdnf_valgrind ${VALGRIND_PROGRAM} --error-exitcode=1 --leak-check=full
        --suppressions=${CMAKE_SOURCE_DIR}/tests/glib.supp
        --suppressions=${CMAKE_SOURCE_DIR}/tests/nss.supp
        --suppressions=${CMAKE_SOURCE_DIR}/tests/giovfs.supp
        --suppressions=${CMAKE_SOURCE_DIR}/tests/librepo.supp
        ${CMAKE_CURRENT_BINARY_DIR}/test_libdnf_main
    )
endif()
