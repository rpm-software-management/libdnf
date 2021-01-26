# print initial information about the project
message("Running CMake on libdnf_snapper_plugin...")
project(libdnf_snapper_plugin C)

# cmake requirements and policies
cmake_minimum_required(VERSION 2.8.5)

find_package(PkgConfig)

pkg_check_modules(LIBDNF REQUIRED libdnf>=0.24.1)

pkg_check_modules(DBUS dbus-1)
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

pkg_check_modules(GLIB REQUIRED gio-unix-2.0>=2.46.0)
include_directories(${GLIB_INCLUDE_DIRS})

add_library(snapper_plugin MODULE snapper_plugin.c snapper_utils.c)

target_link_libraries(snapper_plugin PRIVATE ${LIBDNF_LIBRARIES} ${DBUS_LIBRARIES})
