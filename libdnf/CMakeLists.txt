SET (LIBDNF_SRCS
    dnf-advisorypkg.cpp
    dnf-advisoryref.cpp
    dnf-advisory.cpp
    hy-goal.cpp
    hy-iutil.cpp
    hy-nevra.cpp
    hy-module-form.cpp
    dnf-reldep.cpp
    dnf-reldep-list.cpp
    hy-package.cpp
    hy-packageset.cpp
    hy-query.cpp
    hy-repo.cpp
    dnf-sack.cpp
    hy-selector.cpp
    hy-subject.cpp
    hy-subject-private.cpp
    hy-util.cpp
    dnf-context.cpp
    dnf-goal.cpp
    dnf-keyring.cpp
    dnf-lock.cpp
    dnf-package.cpp
    dnf-packagedelta.cpp
    dnf-repo-loader.cpp
    dnf-rpmts.cpp
    dnf-repo.cpp
    dnf-state.cpp
    dnf-transaction.cpp
    dnf-utils.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
include_directories(${JSONCPP_INCLUDE_DIRS})

INCLUDE_DIRECTORIES (swdb)
ADD_SUBDIRECTORY ("swdb")

configure_file ("dnf-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/dnf-version.h)
configure_file ("libdnf.pc.in" ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc @ONLY)

ADD_SUBDIRECTORY(utils)
ADD_SUBDIRECTORY(sack)

SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${UTILS_SOURCES})
SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${SACK_SOURCES})

ADD_LIBRARY(libdnf SHARED ${LIBDNF_SRCS})
TARGET_LINK_LIBRARIES(libdnf
                      ${REPO_LIBRARIES}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GIO_UNIX_LIBRARIES}
                      ${LIBSOLV_LIBRARY}
                      ${LIBSOLV_EXT_LIBRARY}
                      ${RPMDB_LIBRARY}
                      ${SCOLS_LIBRARIES}
                      ${SQLite3_LIBRARIES}
                      ${JSONCPP_LIBRARIES})

if (ENABLE_RHSM_SUPPORT)
    target_link_libraries (libdnf ${RHSM_LIBRARIES})
endif ()

SET(DNF_SO_VERSION 1)
SET_TARGET_PROPERTIES(libdnf PROPERTIES OUTPUT_NAME "dnf")
SET_TARGET_PROPERTIES(libdnf PROPERTIES SOVERSION ${DNF_SO_VERSION})

SET(LIBHAWKEY_headers
    hy-goal.h
    hy-nevra.hpp
    hy-module-form.h
    hy-package.h
    hy-packageset.h
    hy-query.h
    hy-repo.h
    hy-selector.h
    hy-subject.h
    hy-types.h
    hy-util.h
    )

SET(LIBDNF_headers
    ${LIBDNF_headers}
    dnf-advisory.h
    dnf-advisorypkg.h
    dnf-advisoryref.h
    dnf-context.h
    dnf-enums.h
    dnf-goal.h
    dnf-keyring.h
    dnf-lock.h
    dnf-package.h
    dnf-packagedelta.h
    dnf-repo-loader.h
    dnf-rpmts.h
    dnf-sack.h
    dnf-reldep.h
    dnf-reldep-list.h
    dnf-repo.h
    dnf-state.h
    dnf-transaction.h
    dnf-types.h
    dnf-utils.h
    dnf-version.h
    libdnf.h)

if (NOT PYTHON_DESIRED)
    set (PYTHON_DESIRED "3")
endif()

if (${PYTHON_DESIRED} STREQUAL "2")
    message("Building for python2")
    FIND_PACKAGE (PythonInterp 2 REQUIRED)
    FIND_PACKAGE (PythonLibs 2 REQUIRED)
else()
    message("Building for python3")
    FIND_PACKAGE(PythonInterp 3 REQUIRED)
    FIND_PACKAGE(PythonLibs 3 REQUIRED)
endif()

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc
         DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig)
INSTALL(FILES ${LIBDNF_headers} DESTINATION include/libdnf)
INSTALL(FILES ${LIBHAWKEY_headers} DESTINATION include/libdnf)
INSTALL(TARGETS libdnf LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
