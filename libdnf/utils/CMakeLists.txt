add_subdirectory(bgettext)
add_subdirectory(crypto)
add_subdirectory(iniparser)
add_subdirectory(regex)
add_subdirectory(sqlite3)
add_subdirectory(smartcols)

set(UTILS_SOURCES
    ${UTILS_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/File.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CompressedFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
    PARENT_SCOPE
)

set(UTILS_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PreserveOrderMap.hpp
)

install(FILES ${UTILS_PUBLIC_HEADERS} DESTINATION include/libdnf/utils)
