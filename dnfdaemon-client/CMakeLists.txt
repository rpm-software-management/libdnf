if(NOT WITH_DNFDAEMON_CLIENT)
    return()
endif()


set(DNFDAEMON_CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)


add_executable(dnfdaemon-client main.cpp ${DNFDAEMON_CLIENT_SOURCES})
target_link_libraries(dnfdaemon-client PUBLIC libdnf libdnf-cli)
install(TARGETS dnfdaemon-client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
