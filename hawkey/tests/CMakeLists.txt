SET (hawkeytest_SRCS
     fixtures.c
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

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write('%s.%s' % (sys.version_info.major, sys.version_info.minor))" OUTPUT_VARIABLE PYTHON_MAJOR_DOT_MINOR_VERSION)

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
