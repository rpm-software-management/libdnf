SET (LIBDNF_SRCS
    dnf-advisorypkg.cpp
    dnf-advisoryref.cpp
    dnf-advisory.cpp
    hy-goal.cpp
    hy-iutil.cpp
    log.cpp
    nevra.cpp
    nsvcap.cpp
    dnf-reldep.cpp
    dnf-reldep-list.cpp
    hy-package.cpp
    hy-packageset.cpp
    hy-query.cpp
    hy-repo.cpp
    dnf-sack.cpp
    hy-selector.cpp
    hy-subject.cpp
    hy-util.cpp
    dnf-db.cpp
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

INCLUDE_DIRECTORIES (transaction)
ADD_SUBDIRECTORY ("transaction")

configure_file ("dnf-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/dnf-version.h)
configure_file ("libdnf.pc.in" ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc @ONLY)

ADD_SUBDIRECTORY(goal)
ADD_SUBDIRECTORY(repo)
ADD_SUBDIRECTORY(module)
ADD_SUBDIRECTORY(utils)
ADD_SUBDIRECTORY(conf)
ADD_SUBDIRECTORY(sack)

SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${GOAL_SOURCES})
SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${MODULE_SOURCES})
SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${UTILS_SOURCES})
SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${CONF_SOURCES})
SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${SACK_SOURCES})
SET(LIBDNF_SRCS ${LIBDNF_SRCS} ${REPO_SOURCES})

SET(LIBDNF_headers ${LIBDNF_headers} ${UTILS_headers})

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
                      ${JSONC_LIBRARIES}
                      ${LIBMODULEMD_LIBRARIES})

if (ENABLE_RHSM_SUPPORT)
    target_link_libraries (libdnf ${RHSM_LIBRARIES})
endif ()

SET(DNF_SO_VERSION 2)
SET_TARGET_PROPERTIES(libdnf PROPERTIES OUTPUT_NAME "dnf")
SET_TARGET_PROPERTIES(libdnf PROPERTIES SOVERSION ${DNF_SO_VERSION})

SET(LIBHAWKEY_headers
    hy-goal.h
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
    log.hpp
    nevra.hpp
    nsvcap.hpp
    dnf-advisory.h
    dnf-advisorypkg.h
    dnf-advisoryref.h
    dnf-db.h
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

MESSAGE (STATUS "Building for python${PYTHON_VERSION_MAJOR}")

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc
         DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig)
INSTALL(FILES ${LIBDNF_headers} DESTINATION include/libdnf)
INSTALL(FILES ${LIBHAWKEY_headers} DESTINATION include/libdnf)
INSTALL(TARGETS libdnf LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
