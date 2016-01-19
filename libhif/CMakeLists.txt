SET (LIBHIF_SRCS
    hif-advisorypkg.c
    hif-advisoryref.c
    hif-advisory.c
    hy-goal.c
    hy-iutil.c
    hy-nevra.c
    hy-package.c
    hy-packageset.c
    hy-query.c
    hy-reldep.c
    hy-repo.c
    hif-sack.c
    hy-selector.c
    hy-subject.c
    hy-subject-private.c
    hy-util.c
    hif-context.c
    hif-db.c
    hif-goal.c
    hif-keyring.c
    hif-lock.c
    hif-package.c
    hif-packagedelta.c
    hif-repos.c
    hif-rpmts.c
    hif-repo.c
    hif-state.c
    hif-transaction.c
    hif-utils.c)

CONFIGURE_FILE("hif-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/hif-version.h)
CONFIGURE_FILE("libhif.pc.in" "${CMAKE_CURRENT_SOURCE_DIR}/libhif.pc" @ONLY)

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

SET_TARGET_PROPERTIES(libhif PROPERTIES OUTPUT_NAME "hif")
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

SET(LIBHIF_headers
    hif-advisory.h
    hif-advisorypkg.h
    hif-advisoryref.h
    hif-context.h
    hif-db.h
    hif-goal.h
    hif-keyring.h
    hif-lock.h
    hif-package.h
    hif-packagedelta.h
    hif-repos.h
    hif-rpmts.h
    hif-sack.h
    hif-repo.h
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

INSTALL(FILES "libhif.pc"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig")
INSTALL(FILES ${LIBHIF_headers} DESTINATION include/libhif)
INSTALL(TARGETS libhif LIBRARY DESTINATION ${LIB_INSTALL_DIR})
