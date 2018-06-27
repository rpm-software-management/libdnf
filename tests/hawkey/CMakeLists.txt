SET (hawkeytest_SRCS
     fixtures.cpp
     test_advisory.cpp
     test_advisorypkg.cpp
     test_advisoryref.cpp
     test_goal.cpp
     test_iutil.cpp
     test_main.cpp
     test_packagelist.cpp
     test_packageset.cpp
     test_reldep.cpp
     test_repo.cpp
     test_query.cpp
     test_sack.cpp
     test_selector.cpp
     test_subject.cpp
     test_util.cpp
     testshared.cpp
     testsys.cpp)

ADD_LIBRARY(testshared STATIC testshared.cpp)
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

