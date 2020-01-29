message("Running CMake on libdnf...")
cmake_minimum_required(VERSION 3.0)
project(libdnf LANGUAGES CXX VERSION 1.0.0)


cmake_policy(VERSION ${CMAKE_VERSION})


message("Building ${PROJECT_NAME} version ${PROJECT_VERSION}")


# build options - components (sub-packages)
option(WITH_DNFDAEMON_SERVER "Build DBus dnfdaemon" ON)
option(WITH_DNFDAEMON_CLIENT "Build command-line interface for dnfdaemon" ON)
option(WITH_LIBDNF_CLI "Build libdnf-cli library for handling inputs and outputs in the terminal" ON)
option(WITH_MICRODNF "Build microdnf command-line package manager" ON)

# build options - features
option(WITH_COMPS "Build with comps groups and environments support" ON)
option(WITH_MODULEMD "Build with modulemd modules support" ON)
option(WITH_ZCHUNK "Build with zchunk delta compression support" ON)

# build options - documentation
option(WITH_HTML "Build HTML documentation" ON)
option(WITH_MAN "Build man pages" ON)

# build options - debugging
option(WITH_SANITIZERS "Build with address, leak and undefined sanitizers (DEBUG ONLY)" OFF)

# build options - bindings
option(WITH_GO "Build Go bindings" ON)
option(WITH_PERL5 "Build Perl 5 bindings" ON)
option(WITH_PYTHON3 "Build Python 3 bindings" ON)
option(WITH_RUBY "Build Ruby bindings" ON)


# includes
include(GNUInstallDirs)


# common dependencies
find_package(PkgConfig REQUIRED)


# C++ standard
add_compile_options(-std=c++17)

# warnings
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-Wcast-align -Wformat-nonliteral -Wmissing-format-attribute -Wredundant-decls -Wsign-compare -Wtype-limits -Wuninitialized -Wwrite-strings)
# not sure about the conversion warnings being errors; review later
add_compile_options(-Wconversion)

# linker options
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wl,--as-needed)
endif()


if(WITH_SANITIZERS)
    message(WARNING "Building with sanitizers enabled!")
    set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=leak -fsanitize=undefined")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # requires: compiler-rt package on Fedora
        set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -static-libsan")
    else()
        set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -static-libasan -static-liblsan -static-libubsan")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()


include_directories("${PROJECT_SOURCE_DIR}/include")


# libdnf
add_subdirectory("include")
add_subdirectory("libdnf")
add_subdirectory("libdnf-cli")
add_subdirectory("doc")
add_subdirectory("bindings")

# dnfdaemon
add_subdirectory("dnfdaemon-server")
add_subdirectory("dnfdaemon-client")

# microdnf
add_subdirectory("microdnf")

# tests
enable_testing()
add_subdirectory("test")
