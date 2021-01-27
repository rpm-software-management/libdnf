set(LIBDNF_SRCS
    dnf-advisorypkg.cpp
    dnf-advisoryref.cpp
    dnf-advisory.cpp
    hy-goal.cpp
    hy-iutil.cpp
    log.cpp
    nevra.cpp
    nsvcap.cpp
    dnf-reldep.cpp
    dnf-reldep-list.cpp
    hy-package.cpp
    hy-packageset.cpp
    hy-query.cpp
    dnf-sack.cpp
    hy-selector.cpp
    hy-subject.cpp
    hy-util.cpp
    dnf-db.cpp
    dnf-conf.cpp
    dnf-context.cpp
    dnf-goal.cpp
    dnf-keyring.cpp
    dnf-lock.cpp
    dnf-package.cpp
    dnf-packagedelta.cpp
    dnf-repo-loader.cpp
    dnf-rpmts.cpp
    dnf-repo.cpp
    dnf-state.cpp
    dnf-transaction.cpp
    dnf-utils.cpp
)

include_directories(transaction)
add_subdirectory("transaction")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MULTILIB_ARCH "64")
    configure_file("config-64.h.in" ${CMAKE_CURRENT_SOURCE_DIR}/config-64.h)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(MULTILIB_ARCH "32")
    configure_file("config-64.h.in" ${CMAKE_CURRENT_SOURCE_DIR}/config-32.h)
endif()
configure_file("dnf-version.h.in"  ${CMAKE_CURRENT_SOURCE_DIR}/dnf-version.h)
configure_file("libdnf.pc.in" ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc @ONLY)

add_subdirectory(goal)
add_subdirectory(repo)
add_subdirectory(module)
add_subdirectory(utils)
add_subdirectory(conf)
add_subdirectory(sack)
add_subdirectory(plugin)

set(LIBDNF_SRCS ${LIBDNF_SRCS} ${GOAL_SOURCES})
set(LIBDNF_SRCS ${LIBDNF_SRCS} ${MODULE_SOURCES})
set(LIBDNF_SRCS ${LIBDNF_SRCS} ${UTILS_SOURCES})
set(LIBDNF_SRCS ${LIBDNF_SRCS} ${CONF_SOURCES})
set(LIBDNF_SRCS ${LIBDNF_SRCS} ${SACK_SOURCES})
set(LIBDNF_SRCS ${LIBDNF_SRCS} ${REPO_SOURCES})
set(LIBDNF_SRCS ${LIBDNF_SRCS} ${PLUGIN_SOURCES})

if(ENABLE_STATIC)
    add_library(libdnf STATIC ${LIBDNF_SRCS})
else()
    add_library(libdnf SHARED ${LIBDNF_SRCS})
endif()
target_link_libraries(libdnf
    ${CMAKE_DL_LIBS}
    ${REPO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GIO_UNIX_LIBRARIES}
    ${LIBSOLV_LIBRARY}
    ${LIBSOLV_EXT_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${RPM_LIBRARIES}
    ${SCOLS_LIBRARIES}
    ${SQLite3_LIBRARIES}
    ${JSONC_LIBRARIES}
    ${LIBMODULEMD_LIBRARIES}
    ${SMARTCOLS_LIBRARIES}
    ${GPGME_VANILLA_LIBRARIES}
)

if(ENABLE_RHSM_SUPPORT)
    target_link_libraries(libdnf ${RHSM_LIBRARIES})
endif()

set(DNF_SO_VERSION 2)
set_target_properties(libdnf PROPERTIES OUTPUT_NAME "dnf")
set_target_properties(libdnf PROPERTIES SOVERSION ${DNF_SO_VERSION})

set(LIBHAWKEY_headers
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

set(LIBDNF_headers
    config.h
    log.hpp
    nevra.hpp
    nsvcap.hpp
    dnf-advisory.h
    dnf-advisorypkg.h
    dnf-advisoryref.h
    dnf-db.h
    dnf-conf.h
    dnf-context.h
    dnf-enums.h
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
    dnf-state.h
    dnf-transaction.h
    dnf-types.h
    dnf-utils.h
    dnf-version.h
    libdnf.h
)

message(STATUS "Building for python${PYTHON_VERSION_MAJOR}")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libdnf.pc
    DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig
)
install(FILES ${LIBDNF_headers} DESTINATION include/libdnf)
install(FILES ${LIBHAWKEY_headers} DESTINATION include/libdnf)
install(TARGETS libdnf LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
install(FILES "README.plugins" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/libdnf/plugins" RENAME "README")
