ADD_SUBDIRECTORY (solvable)

SET (REPO_SOURCES ${REPO_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/DependencySplitter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackage.cpp
        PARENT_SCOPE)
