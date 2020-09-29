set(hawkeytest_SRCS
    fixtures.cpp
    test_advisory.cpp
    test_advisorypkg.cpp
    test_advisoryref.cpp
    test_goal.cpp
    test_iutil.cpp
    test_main.cpp
    test_package.cpp
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
    testsys.cpp
)

add_library(testshared STATIC testshared.cpp)
set_target_properties(testshared PROPERTIES COMPILE_FLAGS -fPIC)

add_executable(test_hawkey_main ${hawkeytest_SRCS})
# The binary links in testshared which was built with -fPIC
set_target_properties(test_hawkey_main PROPERTIES COMPILE_FLAGS -fPIC)
target_link_libraries(test_hawkey_main
    libdnf
    ${CHECK_LDFLAGS}
    ${CHECK_STATIC_LIBRARIES}
    ${CHECK_STATIC_LDFLAGS}
    ${SOLV_LIBRARY}
    ${SOLVEXT_LIBRARY}
    ${RPMDB_LIBRARY}
)
add_test(test_hawkey_main test_hawkey_main "${CMAKE_CURRENT_SOURCE_DIR}/data/tests/hawkey/")
set_property(TEST test_hawkey_main PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libdnf")
