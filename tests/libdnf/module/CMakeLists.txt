SET (LIBDNF_TEST_SOURCES
        ${LIBDNF_TEST_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/ContextTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModulePackageContainerTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModuleTest.cpp
        PARENT_SCOPE
        )

SET (LIBDNF_TEST_HEADERS
        ${LIBDNF_TEST_HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/ContextTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModulePackageContainerTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModuleTest.hpp
        PARENT_SCOPE
        )
