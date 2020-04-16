SET(CLIENT_BIN dnf-repoconfig-daemon-client)
SET(CLIENT_SRCS
    client.cpp
)

find_package(sdbus-c++ REQUIRED)

add_executable(${CLIENT_BIN} ${CLIENT_SRCS})
target_link_libraries(
    ${CLIENT_BIN}
    PRIVATE
        sdbus-c++
)
install(TARGETS ${CLIENT_BIN} RUNTIME DESTINATION bin)
