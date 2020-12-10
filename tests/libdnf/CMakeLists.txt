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

# ASAN_OPTIONS fix the "ASan runtime does not come first in initial library
# list" message. Note the better solution should be to use LD_PRELOAD=`ls
# /lib64/libasan.so.?`. With the solution used here the sanitizer library is
# loaded when the DSO linked against it is loaded. For any calls up to that
# point the sanitizers aren't active. While hackish, this is convenient because
# e.g. python is causing some leaks during module loading which this works
# around.
set_property(TEST test_libdnf_main PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/libdnf;ASAN_OPTIONS=verify_asan_link_order=0")
