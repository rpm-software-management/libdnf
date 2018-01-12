find_package(GtkDoc REQUIRED)

configure_file("version.xml.in"  ${CMAKE_CURRENT_SOURCE_DIR}/version.xml)

if (GTKDOC_FOUND)
        add_custom_command(OUTPUT doc-scan
            COMMAND ${GTKDOC_SCAN_EXE}
                    --source-dir=${CMAKE_SOURCE_DIR}/libdnf
                    --module=libdnf
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        add_custom_command(OUTPUT doc-mkdb
            COMMAND ${GTKDOC_MKDB_EXE}
                    --sgml-mode
                    --output-format=xml
                    --module=libdnf
                    --source-dir=${CMAKE_SOURCE_DIR}/libdnf
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        add_custom_command(OUTPUT doc-mkhtml
            COMMAND ${GTKDOC_MKHTML_EXE}
                    libdnf
                    ${CMAKE_SOURCE_DIR}/docs/libdnf/libdnf-docs.sgml
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/html")
    add_custom_target(doc-gtk DEPENDS doc-scan doc-mkdb doc-mkhtml)
else ()
    message (FATAL_ERROR "gtk-doc not found")
endif ()

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html DESTINATION share/gtk-doc/html/libdnf)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py)

ADD_CUSTOM_TARGET (doxygen
    doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Building doxygen xml files")
ADD_CUSTOM_TARGET (sphinx-html
    sphinx-build -c ${CMAKE_CURRENT_BINARY_DIR} -b html
                 ${CMAKE_CURRENT_SOURCE_DIR}
                 ${CMAKE_CURRENT_BINARY_DIR}/_build
    COMMENT "Building html documentation")
ADD_CUSTOM_TARGET (doc)
ADD_DEPENDENCIES (sphinx-html doxygen)
ADD_DEPENDENCIES (doc sphinx-html)
