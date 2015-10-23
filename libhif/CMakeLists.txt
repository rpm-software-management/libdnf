SET (hawkey_SRCS
    hif-advisory.c
    hif-advisorypkg.c
    hif-advisoryref.c
    hy-errno.c
    hy-goal.c
    hy-iutil.c
    hy-nevra.c
    hy-package.c
    hy-packageset.c
    hy-query.c
    hy-reldep.c
    hy-repo.c
    hy-sack.c
    hy-selector.c
    hy-subject.c
    hy-subject_internal.c
    hy-util.c)

ADD_LIBRARY(libhawkey SHARED ${hawkey_SRCS})
TARGET_LINK_LIBRARIES(libhawkey ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(libhawkey ${GLIB2_LIBRARIES} ${EXPAT_LIBRARY} ${ZLIB_LIBRARY} ${RPMDB_LIBRARY})
SET_TARGET_PROPERTIES(libhawkey PROPERTIES OUTPUT_NAME "hawkey")
SET_TARGET_PROPERTIES(libhawkey PROPERTIES SOVERSION 3)

ADD_EXECUTABLE(hth hth.c)
TARGET_LINK_LIBRARIES(hth libhawkey
			   ${SOLV_LIBRARY}
			   ${SOLVEXT_LIBRARY}
			   ${GLIB2_LIBRARIES}
			   ${EXPAT_LIBRARY}
			   ${ZLIB_LIBRARY}
			   ${RPMDB_LIBRARY})

ADD_SUBDIRECTORY(python)

SET(HAWKEY_headers
    hif-advisory.h
    hif-advisorypkg.h
    hif-advisoryref.h
    hy-errno.h
    hy-goal.h
    hy-nevra.h
    hy-package.h
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
