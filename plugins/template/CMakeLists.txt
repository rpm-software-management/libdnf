project(libdnf_template_plugin LANGUAGES C)

cmake_minimum_required(VERSION 2.8.5)

# set plugin source files
set(source template_plugin.c)

# add compiler options
add_compile_options(-fPIC -shared -Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Werror=implicit-function-declaration -Wl,--as-needed -Wmissing-prototypes -Waggregate-return -Wshadow)

# first find PkgConfig to enable the use of pkg_check_modules
find_package(PkgConfig REQUIRED)

# then find and include libdnf
pkg_check_modules(LIBDNF REQUIRED libdnf)
include_directories(${LIBDNF_INCLUDE_DIRS})

# creates libtemplate_plugin.so 
add_library(template_plugin SHARED ${source})

# Remove 'lib' prefix from out example plugin
SET_TARGET_PROPERTIES(template_plugin PROPERTIES PREFIX "")

# libdnf plugins are by default read from: /usr/lib64/libdnf/plugins
# we want to suppress the /usr/local prefix
set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
include(GNUInstallDirs)
# this command enables 'make install' to install the plugin
install(TARGETS template_plugin
    LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/libdnf/plugins"
)
