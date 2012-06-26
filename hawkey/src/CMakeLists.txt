SET (hawkey_SRCS
    repo.c goal.c iutil.c package.c packagelist.c query.c sack.c util.c)

ADD_LIBRARY(libhawkey SHARED ${hawkey_SRCS})
TARGET_LINK_LIBRARIES(libhawkey ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(libhawkey ${EXPAT_LIBRARY} ${ZLIB_LIBRARY} ${RPMDB_LIBRARY})
SET_TARGET_PROPERTIES(libhawkey PROPERTIES OUTPUT_NAME "hawkey")
SET_TARGET_PROPERTIES(libhawkey PROPERTIES SOVERSION 0)

ADD_EXECUTABLE(hth hth.c)
TARGET_LINK_LIBRARIES(hth libhawkey
			   ${SOLV_LIBRARY}
			   ${SOLVEXT_LIBRARY}
			   ${EXPAT_LIBRARY}
			   ${ZLIB_LIBRARY}
			   ${RPMDB_LIBRARY})

ADD_SUBDIRECTORY(python)

SET(HAWKEY_headers
    goal.h
    package.h
    packagelist.h
    query.h
    repo.h
    sack.h
    types.h
    util.h)

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL(FILES ${HAWKEY_headers} DESTINATION include/hawkey)
INSTALL(TARGETS libhawkey LIBRARY DESTINATION ${LIB_INSTALL_DIR})

CONFIGURE_FILE("hawkey.pc.cmake" "${CMAKE_BINARY_DIR}/hawkey.pc" @ONLY)
INSTALL(FILES "${CMAKE_BINARY_DIR}/hawkey.pc"
	DESTINATION "${LIB_INSTALL_DIR}/pkgconfig")
