# print initial information about the project
message("Running CMake on libdnf...")
project(libdnf C CXX)


# cmake requirements and policies
cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0005 OLD)
# Avoid a warning because "hth" links to
# the in-tree libhawkey, but uses pkg-config to find
# GLib.  There may be a better way to do this...
cmake_policy(SET CMP0003 NEW)
include(GNUInstallDirs)


# use project specific cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
if(${CMAKE_VERSION} VERSION_LESS 3)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/modules-cmake-2)
endif()


# print exact name and version of the compiled project
include(VERSION.cmake)
message("Building ${PROJECT_NAME} version: ${LIBDNF_VERSION}")


# build options
option(WITH_BINDINGS "Enables python/SWIG bindings" ON)
option(WITH_GTKDOC "Enables libdnf GTK-Doc HTML documentation" ON)
option(WITH_HTML "Enables hawkey HTML generation" ON)
option(WITH_MAN "Enables hawkey man page generation" ON)
option(WITH_ZCHUNK "Build with zchunk support" ON)
option(WITH_TESTS "Build tests" ON)
option(ENABLE_RHSM_SUPPORT "Build with Red Hat Subscription Manager support?" OFF)
option(ENABLE_SOLV_URPMREORDER "Build with support for URPM-like solution reordering?" OFF)


# load pkg-config first; it's required by other modules
find_package(PkgConfig REQUIRED)
if(APPLE)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};/usr/local/share/cmake/Modules/)
    include_directories(/usr/local/include)
endif()


# build dependencies
find_package(Gpgme REQUIRED)
pkg_check_modules(LibSolv REQUIRED libsolv REQUIRED)
pkg_check_modules(LibSolvExt REQUIRED libsolvext REQUIRED)
find_package(OpenSSL REQUIRED)


# build dependencies via pkg-config
if(WITH_TESTS)
    pkg_check_modules(CHECK REQUIRED check)
endif()
pkg_check_modules(GLIB REQUIRED gio-unix-2.0>=2.46.0)
include_directories(${GLIB_INCLUDE_DIRS})
pkg_check_modules(JSONC REQUIRED json-c)
include_directories(${JSONC_INCLUDE_DIRS})
pkg_check_modules(LIBMODULEMD REQUIRED modulemd<2)
pkg_check_modules(REPO REQUIRED librepo)
include_directories(${REPO_INCLUDE_DIRS})
link_directories(${REPO_LIBRARY_DIRS})
pkg_check_modules(RPM REQUIRED rpm>=4.15.0)
pkg_check_modules(SMARTCOLS REQUIRED smartcols)
pkg_check_modules(SQLite3 REQUIRED sqlite3)

# always enable linking with libdnf utils
include_directories(${CMAKE_SOURCE_DIR} libdnf/utils/)

if (WITH_ZCHUNK)
    pkg_check_modules(ZCHUNKLIB zck>=0.9.11 REQUIRED)
    set (CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS} -DWITH_ZCHUNK")
    set (CMAKE_CXX_FLAGS_DEBUG    "${CMAKE_CXX_FLAGS_DEBUG} -DWITH_ZCHUNK")
endif ()

if(ENABLE_RHSM_SUPPORT)
    pkg_check_modules(RHSM REQUIRED librhsm>=0.0.3)
    include_directories(${RHSM_INCLUDE_DIRS})
endif()


# glibc: check if fnmatch.h has FNM_CASEFOLD symbol
include(CheckSymbolExists)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(FNM_CASEFOLD "fnmatch.h" HAS_FNM_CASEFOLD)
if(NOT HAS_FNM_CASEFOLD)
    message(SEND_ERROR "FNM_CASEFOLD is not available in fnmatch.h")
endif()

# python
if(WITH_BINDINGS)
    if(NOT PYTHON_DESIRED)
        find_package(PythonInterp REQUIRED)
    elseif(${PYTHON_DESIRED} STREQUAL "2")
        find_package(PythonInterp 2 EXACT REQUIRED)
    elseif(${PYTHON_DESIRED} STREQUAL "3")
        find_package(PythonInterp 3 EXACT REQUIRED)
    elseif(EXISTS ${PYTHON_DESIRED})
        set(PYTHON_EXECUTABLE ${PYTHON_DESIRED})
        find_package(PythonInterp REQUIRED)
    else()
        message(FATAL_ERROR "Invalid PYTHON_DESIRED value: " ${PYTHON_DESIRED})
    endif()
    find_package(PythonLibs REQUIRED)
endif()


# valgrind path
find_program(VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin)


# compiler options
add_compile_options(-Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Werror=implicit-function-declaration -Wl,--as-needed)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wmissing-prototypes -Waggregate-return -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wmissing-declarations")

# apple: turn rpath off
set(CMAKE_MACOSX_RPATH 0)

# package/project version
add_definitions(-DPACKAGE_VERSION=\\"${LIBDNF_VERSION}\\")

# The libdnf API is under development now. This enables it for internal usage.
add_definitions(-DLIBDNF_UNSTABLE_API)

# gettext
add_definitions(-DGETTEXT_DOMAIN=\\"libdnf\\")
add_definitions(-DG_LOG_DOMAIN=\\"libdnf\\")

# gpgme: LargeFile Support is required on 32bit architectures
add_definitions(-D_FILE_OFFSET_BITS=64)

# tests
add_definitions(-DTESTDATADIR=\\"${CMAKE_SOURCE_DIR}/data/tests\\")

# librhsm
if(ENABLE_RHSM_SUPPORT)
    add_definitions(-DRHSM_SUPPORT)
endif()

# libsolv
if(ENABLE_SOLV_URPMREORDER)
    add_definitions(-DLIBSOLV_FLAG_URPMREORDER=1)
endif()


# build binaries
add_subdirectory(libdnf)
if(WITH_BINDINGS)
    # add_subdirectory(bindings/perl)
    add_subdirectory(bindings/python)
    add_subdirectory(python/hawkey)
endif()


# build translations
add_subdirectory(po)


# build docs
add_subdirectory(docs/libdnf)
if(WITH_BINDINGS)
    add_subdirectory(docs/hawkey)
endif()

if(WITH_TESTS)
    # build tests
    enable_testing()
    add_subdirectory(tests)
endif()



set(CPACK_PACKAGE_NAME libdnf)
set(CPACK_PACKAGE_VERSION_MAJOR "${LIBDNF_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${LIBDNF_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${LIBDNF_PATCH_VERSION}")
set(CPACK_PACKAGE_VENDOR "Red Hat")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software management library.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/CPack.libdnf.description.txt")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/rpm-software-management/libdnf")
set(CPACK_PACKAGE_CONTACT "https://github.com/rpm-software-management/librepo")
set(CPACK_PACKAGE_CHECKSUM "SHA512")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_GENERATOR "DEB")

message(STATUS "GLIB_VERSION ${GLIB_VERSION}")
string(REPLACE "." ";" GLIB2_VERSION_PARSED "${GLIB_VERSION}")
list(GET GLIB2_VERSION_PARSED 0 GLIB_VERSION_MAJOR)
list(GET GLIB2_VERSION_PARSED 1 GLIB_VERSION_MINOR)

string(REPLACE "." ";" GPGME_VERSION_PARSED "${GPGME_VERSION}")
list(GET GPGME_VERSION_PARSED 0 GPGME_VERSION_MAJOR)

string(REPLACE "." ";" LIBCRYPTO_VERSION_PARSED "${OPENSSL_VERSION}")
list(GET LIBCRYPTO_VERSION_PARSED 0 LIBCRYPTO_VERSION_MAJOR)
list(GET LIBCRYPTO_VERSION_PARSED 1 LIBCRYPTO_VERSION_MINOR)

string(REPLACE "." ";" LibSolv_VERSION_PARSED "${LibSolv_VERSION}")
list(GET LibSolv_VERSION_PARSED 0 LibSolv_VERSION_MAJOR)
list(GET LibSolv_VERSION_PARSED 1 LibSolv_VERSION_MINOR)

string(REPLACE "." ";" LibSolvExt_VERSION_PARSED "${LibSolvExt_VERSION}")
list(GET LibSolvExt_VERSION_PARSED 0 LibSolvExt_VERSION_MAJOR)
list(GET LibSolvExt_VERSION_PARSED 1 LibSolvExt_VERSION_MINOR)

string(REPLACE "." ";" SQLite3_VERSION_PARSED "${SQLite3_VERSION}")
list(GET SQLite3_VERSION_PARSED 0 SQLite3_VERSION_MAJOR)

string(REPLACE "." ";" LIBMODULEMD_VERSION_PARSED "${LIBMODULEMD_VERSION}")
list(GET LIBMODULEMD_VERSION_PARSED 0 LIBMODULEMD_VERSION_MAJOR)


string(REPLACE "." ";" SMARTCOLS_VERSION_PARSED "${SMARTCOLS_VERSION}")
list(GET SMARTCOLS_VERSION_PARSED 0 SMARTCOLS_VERSION_MAJOR)
list(GET SMARTCOLS_VERSION_PARSED 1 SMARTCOLS_VERSION_MINOR)


set(CPACK_DEBIAN_PACKAGE_DEPENDS "libglib${GLIB_VERSION_MAJOR}.0-0 (>=${GLIB_VERSION}~);libgpgme${GPGME_VERSION_MAJOR}1 (>=${GPGME_VERSION}~);libssl${LIBCRYPTO_VERSION_MAJOR}.${LIBCRYPTO_VERSION_MINOR} (>=${OPENSSL_VERSION}~);libjson-c4 (>=${JSONC_VERSION}~);librpm8 (>=${RPM_VERSION}~);librepo (>=${REPO_VERSION});libsolv${LibSolv_VERSION_MAJOR} (>=${LibSolv_VERSION}~);libsolvext${LibSolvExt_VERSION_MAJOR} (>=${LibSolvExt_VERSION}~);libsqlite${SQLite3_VERSION_MAJOR}-0 (>=${SQLite3_VERSION}~);libmodulemd${LIBMODULEMD_VERSION_MAJOR} (>=${LIBMODULEMD_VERSION}~);libsmartcols1 (>=${SMARTCOLS_VERSION_MAJOR}.${SMARTCOLS_VERSION_MINOR}~)")

if(ENABLE_RHSM_SUPPORT)
	list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS "librhsm (>=${RHSM_VERSION}~)")
endif()

if(WITH_ZCHUNK)
	list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS "libzchunk (>=${ZCHUNKLIB_VERSION}~)")
endif()

if(WITH_BINDINGS)
	if(PYTHON_DESIRED)
		list(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS "libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} (>=${PYTHON_VERSION_STRING}~);python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} (>=${PYTHON_VERSION_STRING}~)")
	endif()
endif()

list(JOIN CPACK_DEBIAN_PACKAGE_DEPENDS ", " CPACK_DEBIAN_PACKAGE_DEPENDS)

set(CPACK_DEBIAN_PACKAGE_PROVIDES "libdnf, libdnf-dev, libdnf, python${PYTHON_VERSION_MAJOR}-libdnf")

include(CPack)