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
option(ENABLE_STATIC "Build a static library instead of shared" OFF)
option(WITH_GTKDOC "Enables libdnf GTK-Doc HTML documentation" ON)
option(WITH_HTML "Enables hawkey HTML generation" ON)
option(WITH_MAN "Enables hawkey man page generation" ON)
option(WITH_ZCHUNK "Build with zchunk support" ON)
option(ENABLE_RHSM_SUPPORT "Build with Red Hat Subscription Manager support?" OFF)
option(ENABLE_SOLV_URPMREORDER "Build with support for URPM-like solution reordering?" OFF)


# build options - debugging
option(WITH_SANITIZERS "Build with address, leak and undefined sanitizers (DEBUG ONLY)" OFF)


# load pkg-config first; it's required by other modules
find_package(PkgConfig REQUIRED)
if(APPLE)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};/usr/local/share/cmake/Modules/)
    include_directories(/usr/local/include)
endif()


# build dependencies
find_package(Gpgme REQUIRED)
find_package(LibSolv 0.6.30 REQUIRED COMPONENTS ext)
find_package(OpenSSL REQUIRED)


# build dependencies via pkg-config
pkg_check_modules(CHECK REQUIRED check)
pkg_check_modules(GLIB REQUIRED gio-unix-2.0>=2.46.0)
include_directories(${GLIB_INCLUDE_DIRS})
pkg_check_modules(JSONC REQUIRED json-c)
include_directories(${JSONC_INCLUDE_DIRS})
pkg_check_modules(LIBMODULEMD REQUIRED modulemd-2.0>=2.11.2)
pkg_check_modules(REPO REQUIRED librepo>=1.12.0)
include_directories(${REPO_INCLUDE_DIRS})
link_directories(${REPO_LIBRARY_DIRS})
pkg_check_modules(RPM REQUIRED rpm>=4.11.0)
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


# compiler options
add_compile_options(-Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Wl,--as-needed)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wmissing-prototypes -Waggregate-return -Wshadow -Werror=implicit-function-declaration")
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


if(WITH_SANITIZERS)
    message(WARNING "Building with sanitizers enabled!")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fsanitize=leak)
    link_libraries(asan ubsan)
endif()


# build binaries
add_subdirectory(libdnf)
if(WITH_BINDINGS)
    # add_subdirectory(bindings/perl)
    add_subdirectory(bindings/python)
endif()


# build translations
add_subdirectory(po)


# build docs
add_subdirectory(docs/libdnf)
if(WITH_BINDINGS)
    add_subdirectory(docs/hawkey)
endif()


# build tests
enable_testing()
add_subdirectory(tests)
if(WITH_BINDINGS)
    add_subdirectory(python/hawkey)
endif()


add_subdirectory(etc)
