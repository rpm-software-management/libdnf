ADD_SUBDIRECTORY (bgettext)
ADD_SUBDIRECTORY (iniparser)
ADD_SUBDIRECTORY (regex)
ADD_SUBDIRECTORY (sqlite3)

SET (UTILS_SOURCES
        ${UTILS_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/filesystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/File.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CompressedFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp
        PARENT_SCOPE)

SET (UTILS_headers
        ${UTILS_headers}
        ${CMAKE_CURRENT_SOURCE_DIR}/logger.hpp
        PARENT_SCOPE)
