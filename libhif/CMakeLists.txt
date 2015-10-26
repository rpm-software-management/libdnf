SET (hawkey_SRCS
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
# TODO make this staff generic using FILE(GLOB ...)

CONFIGURE_FILE("hif-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/hif-version.h)

ADD_LIBRARY(libhawkey SHARED ${hawkey_SRCS})
TARGET_LINK_LIBRARIES(libhawkey ${GLIB_LIBRARIES})
        
TARGET_LINK_LIBRARIES(libhawkey ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(libhawkey ${EXPAT_LIBRARY} ${ZLIB_LIBRARY} ${RPMDB_LIBRARY})
TARGET_LINK_LIBRARIES(libhawkey ${INTROSPECTION_LIBS})
SET_TARGET_PROPERTIES(libhawkey PROPERTIES OUTPUT_NAME "hawkey")
SET_TARGET_PROPERTIES(libhawkey PROPERTIES SOVERSION 3)

ADD_EXECUTABLE(hth hy-hth.c)
TARGET_LINK_LIBRARIES(hth libhawkey
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
    hy-errno.h
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

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL(FILES ${HAWKEY_headers} DESTINATION include/hawkey)
INSTALL(TARGETS libhawkey LIBRARY DESTINATION ${LIB_INSTALL_DIR})
