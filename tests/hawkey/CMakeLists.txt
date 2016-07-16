SET (hawkeytest_SRCS
     fixtures.c
     test_advisory.c
     test_advisorypkg.c
     test_advisoryref.c
     test_goal.c
     test_iutil.c
     test_main.c
     test_package.c
     test_packagelist.c
     test_packageset.c
     test_reldep.c
     test_repo.c
     test_query.c
     test_sack.c
     test_selector.c
     test_subject.c
     test_util.c
     testshared.c
     testsys.c)

ADD_LIBRARY(testshared STATIC testshared.c)
SET_TARGET_PROPERTIES(testshared PROPERTIES COMPILE_FLAGS -fPIC)

ADD_EXECUTABLE(test_hawkey_main ${hawkeytest_SRCS})
TARGET_LINK_LIBRARIES(test_hawkey_main
    libdnf
    ${CHECK_LIBRARIES}
    ${SOLV_LIBRARY}
    ${SOLVEXT_LIBRARY}
    ${RPMDB_LIBRARY})
ADD_TEST(test_hawkey_main test_hawkey_main "${CMAKE_CURRENT_SOURCE_DIR}/data/tests/hawkey/")
IF (NOT DISABLE_VALGRIND AND VALGRIND_PROGRAM)
    ADD_TEST(test_hawkey_valgrind ${VALGRIND_PROGRAM} --error-exitcode=1 --leak-check=full
             --suppressions=${CMAKE_SOURCE_DIR}/tests/glib.supp
             --suppressions=${CMAKE_SOURCE_DIR}/tests/nss.supp
             --suppressions=${CMAKE_SOURCE_DIR}/tests/giovfs.supp
             --suppressions=${CMAKE_SOURCE_DIR}/tests/librepo.supp
             ${CMAKE_CURRENT_BINARY_DIR}/test_hawkey_main
             ${CMAKE_CURRENT_SOURCE_DIR}/data/tests/hawkey/)
    SET_TESTS_PROPERTIES(test_hawkey_valgrind PROPERTIES ENVIRONMENT "CK_FORK=no")
ENDIF()

