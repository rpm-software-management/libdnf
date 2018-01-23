PROJECT (libdnf C CXX)
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.5)
CMAKE_POLICY (SET CMP0005 OLD)
# Avoid a warning because "hth" links to
# the in-tree libhawkey, but uses pkg-config to find
# GLib.  There may be a better way to do this...
CMAKE_POLICY(SET CMP0003 NEW)

SET(CMAKE_MACOSX_RPATH 0)

include (VERSION.cmake)
message ("Building libdnf version: ${LIBDNF_VERSION}")

ADD_COMPILE_OPTIONS (-Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Winline -Wall -Werror=implicit-function-declaration -Wl,--as-needed)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wmissing-prototypes -Waggregate-return -Wshadow")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wmissing-declarations")

include (CheckSymbolExists)
list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists (FNM_CASEFOLD "fnmatch.h" HAS_FNM_CASEFOLD)
if (NOT HAS_FNM_CASEFOLD)
  message (SEND_ERROR "FNM_CASEFOLD is not available")
endif ()

ADD_DEFINITIONS(-DGETTEXT_DOMAIN=\\"libdnf\\")
ADD_DEFINITIONS(-DG_LOG_DOMAIN=\\"libdnf\\")

INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR} libdnf/utils/)

OPTION(WITH_GTKDOC "Enables libdnf GTK-Doc HTML documentation" ON)
OPTION(WITH_MAN "Enables hawkey man page generation" ON)

OPTION (ENABLE_SOLV_URPMREORDER "Build with support for URPM-like solution reordering?" OFF)
option (ENABLE_RHSM_SUPPORT "Build with Red Hat Subscription Manager support?" OFF)

OPTION (WITH_SWDB "Support for unified software database" ON)

# hawkey dependencies
find_package (PkgConfig REQUIRED)
SET (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
if (APPLE)
    SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};/usr/local/share/cmake/Modules/)
endif(APPLE)
PKG_CHECK_MODULES(GLIB gio-unix-2.0>=2.46.0 REQUIRED)
FIND_LIBRARY (RPMDB_LIBRARY NAMES rpmdb)
find_package (LibSolv 0.6.30 REQUIRED COMPONENTS ext)
if (ENABLE_RHSM_SUPPORT)
    pkg_check_modules (RHSM REQUIRED librhsm)
    include_directories (${RHSM_INCLUDE_DIRS})
    add_definitions (-DRHSM_SUPPORT)
endif ()
pkg_check_modules (CHECK REQUIRED check)
pkg_check_modules (REPO REQUIRED librepo)

FIND_PROGRAM (VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin)

# TODO handle yumdb / dnfdb for libdnf
ADD_DEFINITIONS(-DBUILDOPT_USE_DNF_YUMDB=1)

IF (ENABLE_SOLV_URPMREORDER)
   ADD_DEFINITIONS(-DLIBSOLV_FLAG_URPMREORDER=1)
ENDIF()

if (APPLE)
    INCLUDE_DIRECTORIES(/usr/local/include)
endif(APPLE)
INCLUDE_DIRECTORIES(${REPO_INCLUDE_DIRS})

IF (WITH_SWDB)
   ADD_DEFINITIONS(-DWITH_SWDB=1)
ENDIF()

INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})
link_directories(${REPO_LIBRARY_DIRS})

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

IF (WITH_SWDB)
    pkg_check_modules(SQLite3 sqlite3 REQUIRED)
ENDIF()

IF (WITH_SWDB)
   set (SWDB_PKGCONFIG_DEPS "sqlite3")
ELSE()
   set (SWDB_PKGCONFIG_DEPS "")
ENDIF()

ADD_DEFINITIONS(-DTESTDATADIR=\\"${CMAKE_SOURCE_DIR}/data/tests\\")
ADD_DEFINITIONS(-DPACKAGE_VERSION=\\"${LIBDNF_VERSION}\\")

include (GNUInstallDirs)

ADD_SUBDIRECTORY (libdnf)
ADD_SUBDIRECTORY (po)
ENABLE_TESTING()
ADD_SUBDIRECTORY (tests)
ADD_SUBDIRECTORY (python/hawkey)
ADD_SUBDIRECTORY (docs/hawkey)
ADD_SUBDIRECTORY (docs/libdnf)
