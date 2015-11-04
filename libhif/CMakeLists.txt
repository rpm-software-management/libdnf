SET (LIBHIF_SRCS
    hy-advisory.c
    hy-advisorypkg.c
    hy-advisoryref.c
    hy-goal.c
    hy-iutil.c
    hy-nevra.c
    hy-package.c
    hy-packagelist.c
    hy-packageset.c
    hy-query.c
    hy-reldep.c
    hy-repo.c
    hy-sack.c
    hy-selector.c
    hy-subject.c
    hy-subject-private.c
    hy-util.c
    hif-cmd.c
    hif-context.c
    hif-db.c
    hif-goal.c
    hif-keyring.c
    hif-lock.c
    hif-package.c
    hif-repos.c
    hif-rpmts.c
    hif-sack.c
    hif-source.c
    hif-state.c
    hif-transaction.c
    hif-utils.c)

CONFIGURE_FILE("hif-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/hif-version.h)

ADD_LIBRARY(libhif SHARED ${LIBHIF_SRCS})
TARGET_LINK_LIBRARIES(libhif
                      ${REPO_LIBRARY}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GIO_UNIX_LIBRARIES}
                      ${CHECK_LIBRARY}
                      ${SOLV_LIBRARY}
                      ${SOLVEXT_LIBRARY}
                      ${EXPAT_LIBRARY}
                      ${ZLIB_LIBRARY}
                      ${RPMDB_LIBRARY})

SET_TARGET_PROPERTIES(libhif PROPERTIES OUTPUT_NAME "libhif")
SET_TARGET_PROPERTIES(libhif PROPERTIES SOVERSION 3)

ADD_EXECUTABLE(hth hy-hth.c)
TARGET_LINK_LIBRARIES(hth libhif
                           ${GLIB_LIBRARIES}
                           ${GLIB_GOBJECT_LIBRARIES}
                           ${GLIB_GIO_LIBRARIES}
                           ${GLIB_GIO_UNIX_LIBRARIES}
                           ${SOLV_LIBRARY}
                           ${SOLVEXT_LIBRARY}
                           ${REPO_LIBRARY}
                           ${EXPAT_LIBRARY}
                           ${ZLIB_LIBRARY}
                           ${RPMDB_LIBRARY})

SET(HAWKEY_headers
    hy-advisory.h
    hy-advisorypkg.h
    hy-advisoryref.h
    hy-goal.h
    hy-nevra.h
    hy-package.h
    hy-packagelist.h
    hy-packageset.h
    hy-query.h
    hy-reldep.h
    hy-repo.h
    hy-sack.h
    hy-selector.h
    hy-subject.h
    hy-types.h
    hy-util.h)

SET(LIBHIF_headers
    hif-context.h
    hif-db.h
    hif-goal.h
    hif-keyring.h
    hif-lock.h
    hif-package.h
    hif-repos.h
    hif-rpmts.h
    hif-sack.h
    hif-source.h
    hif-state.h
    hif-transaction.h
    hif-types.h
    hif-utils.h
    hif-version.h
    libhif.h)


IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL(FILES ${HAWKEY_headers} DESTINATION include/hawkey)
INSTALL(FILES ${LIBHIF_headers} DESTINATION include/libhif)
INSTALL(TARGETS libhif LIBRARY DESTINATION ${LIB_INSTALL_DIR})
