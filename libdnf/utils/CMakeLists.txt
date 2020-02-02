set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/number.cpp
)


add_subdirectory("sack")


set(LIBDNF_SOURCES ${LIBDNF_SOURCES} ${SOURCES} PARENT_SCOPE)
