add_subdirectory("utils")


# build libdnf.so
add_library(libdnf SHARED ${LIBDNF_SOURCES})
set(DNF_SO_VERSION 2)
set_target_properties(libdnf PROPERTIES OUTPUT_NAME "dnf")
set_target_properties(libdnf PROPERTIES SOVERSION ${DNF_SO_VERSION})
# required by clang
target_link_libraries(libdnf stdc++)

# install libdnf.so
install(TARGETS libdnf LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
