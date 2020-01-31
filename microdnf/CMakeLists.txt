if(NOT WITH_MICRODNF)
    return()
endif()


set(MICRODNF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)


add_executable(microdnf main.cpp ${DNFDAEMON_CLIENT_SOURCES})
target_link_libraries(microdnf PUBLIC libdnf libdnf-cli)
install(TARGETS microdnf RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
