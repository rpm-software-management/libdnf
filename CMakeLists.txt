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

ADD_COMPILE_OPTIONS (-Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Werror=implicit-function-declaration -Wl,--as-needed)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wmissing-prototypes -Waggregate-return -Wshadow")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wmissing-declarations")

include (CheckSymbolExists)
list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists (FNM_CASEFOLD "fnmatch.h" HAS_FNM_CASEFOLD)
if (NOT HAS_FNM_CASEFOLD)
  message (SEND_ERROR "FNM_CASEFOLD is not available")
endif ()

# LargeFile Support is required by GPGME on 32bit architectures
ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)

ADD_DEFINITIONS(-DGETTEXT_DOMAIN=\\"libdnf\\")
ADD_DEFINITIONS(-DG_LOG_DOMAIN=\\"libdnf\\")

INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR} libdnf/utils/)

OPTION(WITH_GTKDOC "Enables libdnf GTK-Doc HTML documentation" ON)
OPTION(WITH_MAN "Enables hawkey man page generation" ON)

OPTION(WITH_BINDINGS "Enables python/SWIG bindings" ON)

OPTION (ENABLE_SOLV_URPMREORDER "Build with support for URPM-like solution reordering?" OFF)
option (ENABLE_RHSM_SUPPORT "Build with Red Hat Subscription Manager support?" OFF)

# hawkey dependencies
find_package (PkgConfig REQUIRED)
SET (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
IF (${CMAKE_VERSION} VERSION_LESS 3)
    SET (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/modules-cmake-2)
ENDIF ()
if (APPLE)
    SET(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig")
    SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};/usr/local/share/cmake/Modules/)
endif(APPLE)
PKG_CHECK_MODULES(GLIB gio-unix-2.0>=2.46.0 REQUIRED)
pkg_check_modules(SMARTCOLS REQUIRED smartcols)
FIND_LIBRARY (RPMDB_LIBRARY NAMES rpmdb)
find_package (LibSolv 0.6.30 REQUIRED COMPONENTS ext)
if (ENABLE_RHSM_SUPPORT)
    pkg_check_modules (RHSM REQUIRED librhsm>=0.0.3)
    include_directories (${RHSM_INCLUDE_DIRS})
    add_definitions (-DRHSM_SUPPORT)
endif ()
pkg_check_modules (CHECK REQUIRED check)
pkg_check_modules (REPO REQUIRED librepo)
pkg_check_modules (LIBMODULEMD REQUIRED modulemd)

FIND_PROGRAM (VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin)

IF (ENABLE_SOLV_URPMREORDER)
   ADD_DEFINITIONS(-DLIBSOLV_FLAG_URPMREORDER=1)
ENDIF()

if (APPLE)
    INCLUDE_DIRECTORIES(/usr/local/include)
endif(APPLE)
INCLUDE_DIRECTORIES(${REPO_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})
link_directories(${REPO_LIBRARY_DIRS})

IF (WITH_BINDINGS)
IF (NOT PYTHON_DESIRED)
    FIND_PACKAGE (PythonInterp REQUIRED)
ELSEIF (${PYTHON_DESIRED} STREQUAL "2")
    FIND_PACKAGE (PythonInterp 2 EXACT REQUIRED)
ELSEIF (${PYTHON_DESIRED} STREQUAL "3")
    FIND_PACKAGE (PythonInterp 3 EXACT REQUIRED)
ELSEIF (EXISTS ${PYTHON_DESIRED})
    SET (PYTHON_EXECUTABLE ${PYTHON_DESIRED})
    FIND_PACKAGE (PythonInterp REQUIRED)
ELSE ()
    MESSAGE (FATAL_ERROR "Invalid PYTHON_DESIRED value: " ${PYTHON_DESIRED})
ENDIF()
FIND_PACKAGE (PythonLibs REQUIRED)
ENDIF()

# rpm:
FIND_LIBRARY (RPMDB_LIBRARY NAMES rpmdb)
IF (NOT RPMDB_LIBRARY)
  FIND_LIBRARY (RPMDB_LIBRARY NAMES rpm)
ENDIF (NOT RPMDB_LIBRARY)
FIND_LIBRARY (RPMIO_LIBRARY NAMES rpmio)
IF (RPMIO_LIBRARY)
  SET(RPMDB_LIBRARY ${RPMIO_LIBRARY} ${RPMDB_LIBRARY})
ENDIF (RPMIO_LIBRARY)

pkg_check_modules(SQLite3 sqlite3 REQUIRED)

ADD_DEFINITIONS(-DTESTDATADIR=\\"${CMAKE_SOURCE_DIR}/data/tests\\")
ADD_DEFINITIONS(-DPACKAGE_VERSION=\\"${LIBDNF_VERSION}\\")

include (GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC json-c)
include_directories(${JSONC_INCLUDE_DIRS})

ADD_SUBDIRECTORY (libdnf)
ADD_SUBDIRECTORY (po)
IF (WITH_BINDINGS)
# ADD_SUBDIRECTORY (bindings/perl)
ADD_SUBDIRECTORY (bindings/python)
ENDIF()
ENABLE_TESTING()
ADD_SUBDIRECTORY (tests)
IF (WITH_BINDINGS)
ADD_SUBDIRECTORY (python/hawkey)
ADD_SUBDIRECTORY (docs/hawkey)
ENDIF()
ADD_SUBDIRECTORY (docs/libdnf)
