set(SERVER_BIN dnf-repoconfig-daemon)
set(SERVER_SRCS
    main.cpp
    repoconf.cpp
    configuration.cpp
    logger.cpp
)


find_package(sdbus-c++ REQUIRED)
pkg_check_modules(Systemd IMPORTED_TARGET GLOBAL libsystemd)


add_executable(${SERVER_BIN} ${SERVER_SRCS})
set_property(TARGET ${SERVER_BIN} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${SERVER_BIN} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(
    ${SERVER_BIN}
    PRIVATE
        sdbus-c++
        libdnf
        systemd
)
install(TARGETS ${SERVER_BIN} RUNTIME DESTINATION bin)

add_subdirectory(polkit)
add_subdirectory(dbus)
