set(SERVER_BIN dnf-repoconfig-daemon)
set(SERVER_SRCS
    main.cpp
    repoconf.cpp
    configuration.cpp
)


find_package(sdbus-c++ REQUIRED)

add_executable(${SERVER_BIN} ${SERVER_SRCS})
set_property(TARGET ${SERVER_BIN} PROPERTY CXX_STANDARD 17)

target_link_libraries(
    ${SERVER_BIN}
    PRIVATE
        sdbus-c++
        libdnf
)
install(TARGETS ${SERVER_BIN} RUNTIME DESTINATION bin)

add_subdirectory(polkit)
add_subdirectory(dbus)
