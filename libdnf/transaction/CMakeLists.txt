set(TRANSACTION_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CompsEnvironmentItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CompsGroupItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RPMItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Swdb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Transaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MergedTransaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransactionItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Transformer.cpp
)

add_subdirectory(private)

set(LIBDNF_SRCS
    ${LIBDNF_SRCS}
    ${TRANSACTION_SRCS}
    PARENT_SCOPE
)
