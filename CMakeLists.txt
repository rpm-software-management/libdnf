message("Running CMake on libdnf...")
cmake_minimum_required(VERSION 3.0)
project(libdnf LANGUAGES CXX VERSION 1.0.0)


cmake_policy(VERSION ${CMAKE_VERSION})


message("Building ${PROJECT_NAME} version ${PROJECT_VERSION}")


# build options - features
option(WITH_COMPS "Build with comps groups and environments support" ON)
option(WITH_MODULEMD "Build with modulemd modules support" ON)
option(WITH_ZCHUNK "Build with zchunk delta compression support" ON)

# build options - documentation
option(WITH_HTML "Build HTML documentation" ON)
option(WITH_MAN "Build man pages" ON)

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


include_directories("${PROJECT_SOURCE_DIR}/include")


add_subdirectory("include")
add_subdirectory("libdnf")
add_subdirectory("doc")
#add_executable(main main.cpp)
#target_link_libraries(main stdc++)

# build bindings
if(WITH_GO OR WITH_PERL5 OR WITH_PYTHON3 OR WITH_RUBY)
    add_subdirectory("bindings")
endif()


# build tests
enable_testing()
add_subdirectory("test")
