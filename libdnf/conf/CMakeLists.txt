set(CONF_SOURCES
    ${CONF_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionBool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionEnum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionNumber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionPath.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionSeconds.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionString.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionStringList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionBinds.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigMain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigRepo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigParser.cpp
    PARENT_SCOPE
)

set(CONF_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Option.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionChild.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionBool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionEnum.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionNumber.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionPath.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionSeconds.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionString.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionStringList.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OptionBinds.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigMain.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigRepo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigParser.hpp
)

install(FILES ${CONF_PUBLIC_HEADERS} DESTINATION include/libdnf/conf)
