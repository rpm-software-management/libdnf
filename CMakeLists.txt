PROJECT (libhif C)
CMAKE_MINIMUM_REQUIRED (VERSION 2.4)
CMAKE_POLICY (SET CMP0005 OLD)
# Avoid a warning because "hth" links to
# the in-tree libhawkey, but uses pkg-config to find
# GLib.  There may be a better way to do this...
CMAKE_POLICY(SET CMP0003 NEW)

include (VERSION.cmake)
message ("Building libhif version: ${HIF_VERSION}")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wcast-align -Wno-uninitialized -Wmissing-declarations -Wredundant-decls -Wcast-align -Wwrite-strings -Wreturn-type -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Waggregate-return -Wshadow -Winline -Wall -Werror=implicit-function-declaration -Wl,--as-needed")
set (CMAKE_C_FLAGS_DEBUG	"${CMAKE_C_FLAGS} -ggdb -O0")
IF(NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE DEBUG)
ENDIF(NOT CMAKE_BUILD_TYPE)

ADD_DEFINITIONS(-DG_LOG_DOMAIN=\\"libhif\\")

INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR})

OPTION (ENABLE_SOLV_URPMREORDER "Build with support for URPM-like solution reordering?" OFF)

# hawkey dependencies
find_package (PkgConfig REQUIRED)
SET (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
PKG_CHECK_MODULES(GLIB gio-unix-2.0>=2.44.0 REQUIRED)
FIND_LIBRARY (RPMDB_LIBRARY NAMES rpmdb)
find_package (LibSolv 0.6.21 REQUIRED COMPONENTS ext)
pkg_check_modules (CHECK REQUIRED check)
pkg_check_modules (REPO REQUIRED librepo)
FIND_PROGRAM (VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin)

# TODO handle yumdb / dnfdb for libhif
ADD_DEFINITIONS(-DBUILDOPT_USE_DNF_YUMDB=1)

IF (ENABLE_SOLV_URPMREORDER)
   ADD_DEFINITIONS(-DLIBSOLV_FLAG_URPMREORDER=1)
ENDIF()

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

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

ADD_SUBDIRECTORY (libhif)
ENABLE_TESTING()
ADD_SUBDIRECTORY (tests)
ADD_SUBDIRECTORY (python/hawkey)
ADD_SUBDIRECTORY (docs/hawkey)
ADD_SUBDIRECTORY (docs/libhif)
