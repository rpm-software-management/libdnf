SET (LIBHIF_SRCS
    hif-advisorypkg.c
    hif-advisoryref.c
    hif-advisory.c
    hy-goal.c
    hy-iutil.c
    hy-nevra.c
    hif-reldep.c
    hif-reldep-list.c
    hy-package.c
    hy-packageset.c
    hy-query.c
    hy-repo.c
    hif-sack.c
    hy-selector.c
    hy-subject.c
    hy-subject-private.c
    hy-util.c
    hif-context.c
    hif-db.c
    hif-goal.c
    hif-keyring.c
    hif-lock.c
    hif-package.c
    hif-packagedelta.c
    hif-repos.c
    hif-rpmts.c
    hif-repo.c
    hif-state.c
    hif-transaction.c
    hif-utils.c)

configure_file ("hif-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/hif-version.h)
configure_file ("libhif.pc.in" ${CMAKE_CURRENT_BINARY_DIR}/libhif.pc @ONLY)

ADD_LIBRARY(libhif SHARED ${LIBHIF_SRCS})
TARGET_LINK_LIBRARIES(libhif
                      ${REPO_LIBRARIES}
                      ${GLIB_LIBRARIES}
                      ${GLIB_GOBJECT_LIBRARIES}
                      ${GLIB_GIO_LIBRARIES}
                      ${GLIB_GIO_UNIX_LIBRARIES}
                      ${LIBSOLV_LIBRARY}
                      ${LIBSOLV_EXT_LIBRARY}
                      ${RPMDB_LIBRARY})

SET(HIF_SO_VERSION 3)
SET_TARGET_PROPERTIES(libhif PROPERTIES OUTPUT_NAME "hif")
SET_TARGET_PROPERTIES(libhif PROPERTIES SOVERSION ${HIF_SO_VERSION})

ADD_EXECUTABLE(hth hy-hth.c)
TARGET_LINK_LIBRARIES(hth libhif
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
    hy-package.h
    hy-packageset.h
    hy-query.h
    hy-repo.h
    hy-selector.h
    hy-subject.h
    hy-types.h
    hy-util.h
    )

SET(LIBHIF_headers
    hif-advisory.h
    hif-advisorypkg.h
    hif-advisoryref.h
    hif-context.h
    hif-enums.h
    hif-db.h
    hif-goal.h
    hif-keyring.h
    hif-lock.h
    hif-package.h
    hif-packagedelta.h
    hif-repos.h
    hif-rpmts.h
    hif-sack.h
    hif-reldep.h
    hif-reldep-list.h
    hif-repo.h
    hif-state.h
    hif-transaction.h
    hif-types.h
    hif-utils.h
    hif-version.h
    libhif.h)

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

        set(GIR_PREFIX "Hif-${HIF_SO_VERSION}.0")
        set(GIR_XML "${GIR_PREFIX}.gir")
        set(GIR_TYPELIB "${GIR_PREFIX}.typelib")

        add_custom_command(OUTPUT ${GIR_XML}
            COMMAND ${GOBJECT_INTROSPECTION_1.0_G_IR_SCANNER}
                    --namespace=Hif
                    --nsversion=${HIF_SO_VERSION}
                    --library-path=${CMAKE_CURRENT_BINARY_DIR}
                    --library=hif
                    --no-libtool
                    --include=GObject-2.0
                    --include=Gio-2.0
                    -Idirectory ${CMAKE_CURRENT_SOURCE_DIR}
                    --output ${CMAKE_CURRENT_BINARY_DIR}/${GIR_XML}
                    --warn-all
                    --quiet
                    hif-context.c hif-context.h
                    hif-repo.c hif-repo.h
                    hif-state.h hif-state.c
                    hif-version.h
            DEPENDS libhif
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

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libhif.pc
         DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
INSTALL(FILES ${LIBHIF_headers} DESTINATION include/libhif)
INSTALL(FILES ${LIBHAWKEY_headers} DESTINATION include/libhif)
INSTALL(TARGETS libhif LIBRARY DESTINATION ${LIB_INSTALL_DIR})
