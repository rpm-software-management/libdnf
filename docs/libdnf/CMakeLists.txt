configure_file("version.xml.in"  ${CMAKE_CURRENT_SOURCE_DIR}/version.xml)

if(WITH_GTKDOC)
    find_package(GtkDoc REQUIRED)

    if(GTKDOC_FOUND)
        add_custom_command(OUTPUT doc-scan
            COMMAND ${GTKDOC_SCAN_EXE}
            --source-dir=${CMAKE_SOURCE_DIR}/libdnf
            --module=libdnf
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        add_custom_command(OUTPUT doc-mkdb
            COMMAND ${GTKDOC_MKDB_EXE}
            --sgml-mode
            --output-format=xml
            --module=libdnf
            --source-dir=${CMAKE_SOURCE_DIR}/libdnf
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        add_custom_command(OUTPUT doc-mkhtml
            COMMAND ${GTKDOC_MKHTML_EXE}
            libdnf
            ${CMAKE_SOURCE_DIR}/docs/libdnf/libdnf-docs.sgml
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/html"
        )
        add_custom_target(doc-gtk DEPENDS doc-scan doc-mkdb doc-mkhtml)
    else()
        message(FATAL_ERROR "gtk-doc not found")
    endif()

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html DESTINATION share/gtk-doc/html/libdnf)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py)

add_custom_target(doxygen
    doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Building doxygen xml files"
)
add_custom_target(sphinx-html
    sphinx-build -c ${CMAKE_CURRENT_BINARY_DIR} -b html
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/_build
    COMMENT "Building html documentation"
)
add_custom_target(doc)
add_dependencies(sphinx-html doxygen)
add_dependencies(doc sphinx-html)
