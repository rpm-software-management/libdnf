PROJECT (libhif)
CMAKE_MINIMUM_REQUIRED (VERSION 2.4)
CMAKE_POLICY (SET CMP0005 OLD)
# Avoid a warning because "hth" links to
# the in-tree libhawkey, but uses pkg-config to find
# GLib.  There may be a better way to do this...
CMAKE_POLICY(SET CMP0003 NEW)

OPTION(DISABLE_VALGRIND_TESTS "Enables valgrind tests for hawkey and libhif" OFF)
INCLUDE (${CMAKE_SOURCE_DIR}/VERSION.cmake)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wcast-align -Wno-uninitialized -Wmissing-declarations -Wredundant-decls -Wcast-align -Wwrite-strings -Wreturn-type -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Waggregate-return -Wshadow -Winline -Wall -Werror=implicit-function-declaration -Wl,--as-needed")
set (CMAKE_C_FLAGS_DEBUG	"${CMAKE_C_FLAGS} -ggdb -O0")
IF(NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE DEBUG)
ENDIF(NOT CMAKE_BUILD_TYPE)

INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR})
if (LIBSOLV_PATH)
   INCLUDE_DIRECTORIES (${LIBSOLV_PATH}/build/)
   FIND_LIBRARY (SOLV_LIBRARY NAMES solv PATHS ${LIBSOLV_PATH}/build/src NO_DEFAULT_PATH)
   FIND_LIBRARY (SOLVEXT_LIBRARY NAMES solvext PATHS ${LIBSOLV_PATH}/build/ext NO_DEFAULT_PATH)
   ADD_CUSTOM_COMMAND (OUTPUT ${LIBSOLV_PATH}/build/solv
                       COMMAND mkdir -p ${LIBSOLV_PATH}/build/solv
                       COMMAND cp ${LIBSOLV_PATH}/src/*.h ${LIBSOLV_PATH}/build/solv
		       COMMAND cp ${LIBSOLV_PATH}/ext/*.h ${LIBSOLV_PATH}/build/solv
		       COMMAND cp ${LIBSOLV_PATH}/build/src/*.h ${LIBSOLV_PATH}/build/solv)
   ADD_CUSTOM_TARGET (generate_libsolv_includes ALL DEPENDS ${LIBSOLV_PATH}/build/solv)
endif (LIBSOLV_PATH)

# hawkey dependencies
FIND_PACKAGE (PkgConfig)
SET (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
FIND_PACKAGE (GLIB REQUIRED)
FIND_PACKAGE (EXPAT REQUIRED)
FIND_PACKAGE (ZLIB REQUIRED)
FIND_LIBRARY (RPMDB_LIBRARY NAMES rpmdb)
FIND_LIBRARY (SOLV_LIBRARY NAMES solv)
FIND_LIBRARY (SOLVEXT_LIBRARY NAMES solvext)
FIND_LIBRARY (CHECK_LIBRARY NAMES check)
FIND_PROGRAM (VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin)
MESSAGE(STATUS "Using libsolv at ${SOLV_LIBRARY}")

# TODO handle yumdb / dnfdb for libhif
ADD_DEFINITIONS(-DBUILDOPT_USE_DNF_YUMDB=1)

FIND_LIBRARY(REPO_LIBRARY repo REQUIRED)
INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})

if (NOT PYTHON_DESIRED)
    set (PYTHON_DESIRED "2")
endif()

# rpm:
FIND_LIBRARY (RPMDB_LIBRARY NAMES rpmdb)
IF (NOT RPMDB_LIBRARY)
  FIND_LIBRARY (RPMDB_LIBRARY NAMES rpm)
ENDIF (NOT RPMDB_LIBRARY)
FIND_LIBRARY (RPMIO_LIBRARY NAMES rpmio)
IF (RPMIO_LIBRARY)
  SET(RPMDB_LIBRARY ${RPMIO_LIBRARY} ${RPMDB_LIBRARY})
ENDIF (RPMIO_LIBRARY)

ADD_DEFINITIONS(-DTESTDATADIR=\\"${CMAKE_SOURCE_DIR}/data/tests\\")
ADD_DEFINITIONS(-DPACKAGE_VERSION=\\"${HIF_VERSION}\\")

ADD_SUBDIRECTORY (libhif)
ENABLE_TESTING()
ADD_SUBDIRECTORY (tests/hawkey)
ADD_SUBDIRECTORY (python/hawkey)
ADD_SUBDIRECTORY (docs/hawkey)
