SET (LIBDNF_TEST_SOURCES
    ${LIBDNF_TEST_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/CompsEnvironmentItemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RpmItemTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransactionItemReasonTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransactionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WorkflowTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransformerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MergedTransactionTest.cpp
    PARENT_SCOPE
)

SET (LIBDNF_TEST_HEADERS
    ${LIBDNF_TEST_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/CompsEnvironmentItemTest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RpmItemTest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransactionItemReasonTest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransactionTest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WorkflowTest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransformerTest.hpp
    PARENT_SCOPE
)
