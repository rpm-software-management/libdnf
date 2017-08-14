SET (LIBDNF_SRCS
    dnf-advisorypkg.c
    dnf-advisoryref.c
    dnf-advisory.c
    hy-goal.c
    hy-iutil.c
    hy-nevra.c
    hy-module-form.c
    dnf-reldep.c
    dnf-reldep-list.c
    hy-package.c
    hy-packageset.c
    hy-query.c
    hy-repo.c
    dnf-sack.c
    hy-selector.c
    hy-subject.c
    hy-subject-private.c
    hy-util.c
    dnf-context.c
    dnf-db.c
    dnf-goal.c
    dnf-keyring.c
    dnf-lock.c
    dnf-package.c
    dnf-packagedelta.c
    dnf-repo-loader.c
    dnf-rpmts.c
    dnf-repo.c
    dnf-solution.c
    dnf-state.c
    dnf-transaction.c
    dnf-utils.c)

configure_file ("dnf-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/dnf-version.h)
configure_file ("libdnf.pc.in" ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc @ONLY)

ADD_LIBRARY(libdnf SHARED ${LIBDNF_SRCS})
TARGET_LINK_LIBRARIES(libdnf
                      ${REPO_LIBRARIES}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GIO_UNIX_LIBRARIES}
                      ${LIBSOLV_LIBRARY}
                      ${LIBSOLV_EXT_LIBRARY}
                      ${RPMDB_LIBRARY})

if (ENABLE_RHSM_SUPPORT)
    target_link_libraries (libdnf ${RHSM_LIBRARIES})
endif ()

SET(DNF_SO_VERSION 1)
SET_TARGET_PROPERTIES(libdnf PROPERTIES OUTPUT_NAME "dnf")
SET_TARGET_PROPERTIES(libdnf PROPERTIES SOVERSION ${DNF_SO_VERSION})

ADD_EXECUTABLE(hth hy-hth.c)
TARGET_LINK_LIBRARIES(hth libdnf
                      ${GLIB_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GIO_UNIX_LIBRARIES}
                      ${LIBSOLV_LIBRARY}
                      ${LIBSOLV_EXT_LIBRARY}
                      ${REPO_LIBRARIES}
                      ${RPMDB_LIBRARY})

SET(LIBHAWKEY_headers
    hy-goal.h
    hy-nevra.h
    hy-module-form.h
    hy-package.h
    hy-packageset.h
    hy-query.h
    hy-repo.h
    hy-selector.h
    hy-subject.h
    hy-types.h
    hy-util.h
    )

SET(LIBDNF_headers
    dnf-advisory.h
    dnf-advisorypkg.h
    dnf-advisoryref.h
    dnf-context.h
    dnf-enums.h
    dnf-db.h
    dnf-goal.h
    dnf-keyring.h
    dnf-lock.h
    dnf-package.h
    dnf-packagedelta.h
    dnf-repo-loader.h
    dnf-rpmts.h
    dnf-sack.h
    dnf-reldep.h
    dnf-reldep-list.h
    dnf-repo.h
    dnf-solution.h
    dnf-state.h
    dnf-transaction.h
    dnf-types.h
    dnf-utils.h
    dnf-version.h
    libdnf.h)

# gir generation
# taken from https://github.com/ufo-kit/libuca/blob/master/src/CMakeLists.txt
find_program(INTROSPECTION_SCANNER "g-ir-scanner")
find_program(INTROSPECTION_COMPILER "g-ir-compiler")
pkg_check_modules(GOBJECT_INTROSPECTION gobject-introspection-1.0)

find_package(PkgConfig REQUIRED)
function(pkg_check_variable _pkg _name)
    string(TOUPPER ${_pkg} _pkg_upper)
    string(TOUPPER ${_name} _name_upper)
    string(REPLACE "-" "_" _pkg_upper ${_pkg_upper})
    string(REPLACE "-" "_" _name_upper ${_name_upper})
    set(_output_name "${_pkg_upper}_${_name_upper}")

    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=${_name} ${_pkg}
                    OUTPUT_VARIABLE _pkg_result
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    set("${_output_name}" "${_pkg_result}" CACHE STRING "pkg-config variable
    ${_name} of ${_pkg}")
endfunction()

if (GOBJECT_INTROSPECTION_FOUND)
    option(WITH_GIR "Build introspection files" ON)

    if (WITH_GIR)
        pkg_check_variable(gobject-introspection-1.0 g_ir_scanner)
        pkg_check_variable(gobject-introspection-1.0 g_ir_compiler)

        set(GIR_PREFIX "Dnf-${DNF_SO_VERSION}.0")
        set(GIR_XML "${GIR_PREFIX}.gir")
        set(GIR_TYPELIB "${GIR_PREFIX}.typelib")

        add_custom_command(OUTPUT ${GIR_XML}
            COMMAND env CFLAGS=${CMAKE_C_FLAGS} ${GOBJECT_INTROSPECTION_1.0_G_IR_SCANNER}
                    --namespace=Dnf
                    --nsversion=${DNF_SO_VERSION}.0
                    --library-path=${CMAKE_CURRENT_BINARY_DIR}
                    --library=dnf
                    --no-libtool
                    --include=GObject-2.0
                    --include=Gio-2.0
                    -Idirectory ${CMAKE_CURRENT_SOURCE_DIR}
                    --output ${CMAKE_CURRENT_BINARY_DIR}/${GIR_XML}
                    --pkg=librepo
                    --warn-all
                    --quiet
                    dnf-context.c dnf-context.h
                    dnf-repo.c dnf-repo.h
                    dnf-state.h dnf-state.c
                    dnf-version.h
            DEPENDS libdnf
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        add_custom_command(OUTPUT ${GIR_TYPELIB}
            COMMAND ${GOBJECT_INTROSPECTION_1.0_G_IR_COMPILER}
                    -o ${GIR_TYPELIB}
                    ${GIR_XML}
            DEPENDS ${GIR_XML}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        add_custom_target(gir ALL
                          DEPENDS ${GIR_XML} ${GIR_TYPELIB})

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_XML}
                DESTINATION share/gir-1.0
                COMPONENT libraries)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GIR_TYPELIB}
                DESTINATION ${LIB_INSTALL_DIR}/girepository-1.0
                COMPONENT libraries)
    endif()
endif()

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc
         DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
INSTALL(FILES ${LIBDNF_headers} DESTINATION include/libdnf)
INSTALL(FILES ${LIBHAWKEY_headers} DESTINATION include/libdnf)
INSTALL(TARGETS libdnf LIBRARY DESTINATION ${LIB_INSTALL_DIR})
