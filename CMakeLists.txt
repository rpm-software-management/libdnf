project(libdnf C CXX)
cmake_minimum_required(VERSION 2.8.5)
cmake_policy(SET CMP0005 OLD)
# Avoid a warning because "hth" links to
# the in-tree libhawkey, but uses pkg-config to find
# GLib.  There may be a better way to do this...
cmake_policy(SET CMP0003 NEW)

set(CMAKE_MACOSX_RPATH 0)

include(VERSION.cmake)
message("Building libdnf version: ${LIBDNF_VERSION}")

add_compile_options(-Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Werror=implicit-function-declaration -Wl,--as-needed)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wmissing-prototypes -Waggregate-return -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wmissing-declarations")

include(CheckSymbolExists)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(FNM_CASEFOLD "fnmatch.h" HAS_FNM_CASEFOLD)
if(NOT HAS_FNM_CASEFOLD)
    message(SEND_ERROR "FNM_CASEFOLD is not available")
endif()

# LargeFile Support is required by GPGME on 32bit architectures
add_definitions(-D_FILE_OFFSET_BITS=64)

add_definitions(-DGETTEXT_DOMAIN=\\"libdnf\\")
add_definitions(-DG_LOG_DOMAIN=\\"libdnf\\")

# The libdnf API is under development now. This enables it for internal usage.
add_definitions(-DLIBDNF_UNSTABLE_API)

include_directories(${CMAKE_SOURCE_DIR} libdnf/utils/)

option(WITH_GTKDOC "Enables libdnf GTK-Doc HTML documentation" ON)
option(WITH_MAN "Enables hawkey man page generation" ON)
option(WITH_HTML "Enables hawkey HTML generation" ON)

option(WITH_BINDINGS "Enables python/SWIG bindings" ON)

option(ENABLE_SOLV_URPMREORDER "Build with support for URPM-like solution reordering?" OFF)
option(ENABLE_RHSM_SUPPORT "Build with Red Hat Subscription Manager support?" OFF)

# hawkey dependencies
find_package(PkgConfig REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
if(${CMAKE_VERSION} VERSION_LESS 3)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/modules-cmake-2)
endif()
if(APPLE)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};/usr/local/share/cmake/Modules/)
endif()
pkg_check_modules(GLIB gio-unix-2.0>=2.46.0 REQUIRED)
pkg_check_modules(SMARTCOLS REQUIRED smartcols)
pkg_check_modules(RPM REQUIRED rpm>=4.11.0)
find_package(LibSolv 0.6.30 REQUIRED COMPONENTS ext)
if(ENABLE_RHSM_SUPPORT)
    pkg_check_modules(RHSM REQUIRED librhsm>=0.0.3)
    include_directories(${RHSM_INCLUDE_DIRS})
    add_definitions(-DRHSM_SUPPORT)
endif()
pkg_check_modules(CHECK REQUIRED check)
pkg_check_modules(REPO REQUIRED librepo)
pkg_check_modules(LIBMODULEMD REQUIRED modulemd)
find_package(Gpgme REQUIRED)
find_package(OpenSSL REQUIRED)

find_program(VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin)

if(ENABLE_SOLV_URPMREORDER)
    add_definitions(-DLIBSOLV_FLAG_URPMREORDER=1)
endif()

if(APPLE)
    include_directories(/usr/local/include)
endif()
include_directories(${REPO_INCLUDE_DIRS})

include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${REPO_LIBRARY_DIRS})

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

pkg_check_modules(SQLite3 sqlite3 REQUIRED)

add_definitions(-DTESTDATADIR=\\"${CMAKE_SOURCE_DIR}/data/tests\\")
add_definitions(-DPACKAGE_VERSION=\\"${LIBDNF_VERSION}\\")

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC json-c)
include_directories(${JSONC_INCLUDE_DIRS})

add_subdirectory(libdnf)
add_subdirectory(po)
if(WITH_BINDINGS)
    # add_subdirectory(bindings/perl)
    add_subdirectory(bindings/python)
endif()
enable_testing()
add_subdirectory(tests)
if(WITH_BINDINGS)
    add_subdirectory(python/hawkey)
    add_subdirectory(docs/hawkey)
endif()
add_subdirectory(docs/libdnf)
