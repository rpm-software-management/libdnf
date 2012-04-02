SET (hawkeytest_SRCS
     test_goal.c test_iutil.c test_main.c test_package.c test_query.c test_sack.c
     testsys.c)

ADD_LIBRARY(testsys SHARED testsys.c)
ADD_EXECUTABLE(test_main ${hawkeytest_SRCS})
TARGET_LINK_LIBRARIES(test_main
    libhawkey
    ${CHECK_LIBRARY}
    ${SOLV_LIBRARY}
    ${SOLVEXT_LIBRARY}
    ${EXPAT_LIBRARY}
    ${ZLIB_LIBRARY}
    ${RPMDB_LIBRARY})
ADD_TEST(test_main test_main "${CMAKE_CURRENT_SOURCE_DIR}/repos/")

ADD_SUBDIRECTORY (python)
