ADD_EXECUTABLE(test_sack test_sack.c)
TARGET_LINK_LIBRARIES(test_sack
    libhawkey
    ${CHECK_LIBRARY}
    ${SOLV_LIBRARY}
    ${SOLVEXT_LIBRARY}
    ${EXPAT_LIBRARY}
    ${ZLIB_LIBRARY}
    ${RPMDB_LIBRARY})
ADD_TEST(test_sack test_sack "${CMAKE_CURRENT_SOURCE_DIR}/repos/")