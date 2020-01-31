if(NOT WITH_LIBDNF_CLI)
    return()
endif()


add_subdirectory("progressbar")


# build libdnf-cli.so
add_library(libdnf-cli SHARED ${LIBDNF_CLI_SOURCES})
set(DNF_SO_VERSION 0)
set_target_properties(libdnf-cli PROPERTIES OUTPUT_NAME "dnf-cli")
set_target_properties(libdnf-cli PROPERTIES SOVERSION ${DNF_SO_VERSION})
# required by clang
target_link_libraries(libdnf-cli stdc++)

# install libdnf.so
install(TARGETS libdnf-cli LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
