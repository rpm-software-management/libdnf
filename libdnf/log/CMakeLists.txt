SET (LOG_HEADERS
        ${LOG_HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/log.hpp
        PARENT_SCOPE
    )

SET (LOG_SOURCES
        ${LOG_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/file_logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/stderr_logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/stdout_logger.cpp
        PARENT_SCOPE
    )
