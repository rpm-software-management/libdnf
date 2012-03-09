SET (hawkey_SRCS
    frepo.c goal.c iutil.c package.c packagelist.c query.c sack.c)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} --std=c99")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} --std=c99")

INCLUDE_DIRECTORIES(/home/akozumpl/libsolv/src)

ADD_LIBRARY(libhawkey SHARED ${hawkey_SRCS})
TARGET_LINK_LIBRARIES(libhawkey ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(libhawkey ${EXPAT_LIBRARY} ${ZLIB_LIBRARY} ${RPMDB_LIBRARY})
SET_TARGET_PROPERTIES(libhawkey PROPERTIES OUTPUT_NAME "hawkey")

ADD_EXECUTABLE(main main.c)
TARGET_LINK_LIBRARIES(main ${SOLV_LIBRARY}
			   ${SOLVEXT_LIBRARY}
			   ${EXPAT_LIBRARY}
			   ${ZLIB_LIBRARY}
			   ${RPMDB_LIBRARY})

ADD_DEFINITIONS(-DMD_REPO=\\"/home/akozumpl/tmp/md/repomd.xml\\")
ADD_DEFINITIONS(-DMD_PRIMARY_XML=\\"/home/akozumpl/tmp/md/3fffacc6006d9b4782e57f3f11fcb44ab2a1abf6625e4afb4bf6a1e6475dd107-primary.xml.gz\\")
ADD_DEFINITIONS(-DMD_REPO_UPDATES=\\"/home/akozumpl/tmp/md/updates/repomd.xml\\")
ADD_DEFINITIONS(-DMD_PRIMARY_UPDATES_XML=\\"/home/akozumpl/tmp/md/updates/65e967d434400bdcf602da5013668367dae84032c31cb293792884e972a3452d-primary.xml.gz\\")

ADD_SUBDIRECTORY(python)
