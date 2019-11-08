message("Running CMake on libdnf...")
cmake_minimum_required(VERSION 3.0)
project(libdnf LANGUAGES CXX VERSION 1.0.0)
message("Building ${PROJECT_NAME} version ${PROJECT_VERSION}")


# build options - features
option(WITH_COMPS "Build with comps groups and environments support" ON)
option(WITH_MODULEMD "Build with modulemd modules support" ON)
option(WITH_ZCHUNK "Build with zchunk delta compression support" ON)

# build options - documentation
option(WITH_HTML "Build HTML documentation" ON)
option(WITH_MAN "Build man pages" ON)

# build options - bindings
option(WITH_PYTHON3 "Build Python 3 bindings" ON)
option(WITH_PERL5 "Build Perl 5 bindings" ON)


# includes
include(GNUInstallDirs)


# dependencies

add_compile_options(-std=c++17 -Wmissing-declarations -Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Wl,--as-needed)
#add_compile_options(-Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Werr
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wmissing-prototypes -Waggregate-return -Wshadow")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wmissing-declarations")

include_directories("${PROJECT_SOURCE_DIR}/libdnf")
add_subdirectory("doc")
add_executable(main main.cpp)
target_link_libraries(main stdc++)


