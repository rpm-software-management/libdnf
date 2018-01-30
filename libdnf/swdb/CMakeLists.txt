SET(SWDB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/item_comps_environment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/item_comps_group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/item_rpm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/repo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/swdb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mergedtransaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transactionitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transformer.cpp
)

SET(LIBDNF_SRCS
    ${LIBDNF_SRCS}
    ${SWDB_SRCS}
    PARENT_SCOPE
)
