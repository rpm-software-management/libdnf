# pushing/pulling translations to zanata

file(RELATIVE_PATH SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libdnf)
add_custom_target(gettext-export
    find ${SRCDIR} -iname "*.[ch]" -o -iname "*.[ch]pp" |
    xargs xgettext --from-code=UTF-8 --keyword=_ --keyword=M_ --keyword=P_:1,2 --keyword=MP_:1,2 --keyword=C_:1c,2 --keyword=MC_:1c,2 --keyword=CP_:1c,2,3 --keyword=MCP_:1c,2,3 -c --output=libdnf.pot &&
    zanata push -f
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Pushing translation source file to zanata"
)
add_custom_target(gettext-update
    zanata pull
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Updating translation files from zanata"
)

find_package(Gettext)
if(GETTEXT_FOUND)
    # this process unfortunately reformats .po files so copy them
    file(GLOB POS ${CMAKE_CURRENT_SOURCE_DIR}/*.po)
    file(COPY ${POS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(GLOB POS_BIN ${CMAKE_CURRENT_BINARY_DIR}/*.po)
    gettext_create_translations(${CMAKE_CURRENT_SOURCE_DIR}/libdnf.pot ALL ${POS_BIN})
endif()
