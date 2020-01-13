add_subdirectory("utils")

add_library(libdnf SHARED ${LIBDNF_SOURCES})
target_link_libraries(libdnf stdc++)
target_link_libraries(libdnf
    ${CMAKE_DL_LIBS}
)

set(DNF_SO_VERSION 2)
set_target_properties(libdnf PROPERTIES OUTPUT_NAME "dnf")
set_target_properties(libdnf PROPERTIES SOVERSION ${DNF_SO_VERSION})
