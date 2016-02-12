find_package(GtkDoc REQUIRED)

configure_file("version.xml.in"  ${CMAKE_CURRENT_SOURCE_DIR}/version.xml)

if (GTKDOC_FOUND)
        add_custom_command(OUTPUT doc-scan
            COMMAND ${GTKDOC_SCAN_EXE}
                    --source-dir=${CMAKE_SOURCE_DIR}/libhif
                    --module=libhif
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        add_custom_command(OUTPUT doc-mkdb
            COMMAND ${GTKDOC_MKDB_EXE}
                    --sgml-mode
                    --output-format=xml
                    --module=libhif
                    --source-dir=${CMAKE_SOURCE_DIR}/libhif
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        add_custom_command(OUTPUT doc-mkhtml
            COMMAND ${GTKDOC_MKHTML_EXE}
                    libhif
                    ${CMAKE_SOURCE_DIR}/docs/libhif/libhif-docs.sgml
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/html")
    add_custom_target(doc-gtk DEPENDS doc-scan doc-mkdb doc-mkhtml)
else ()
    message (FATAL_ERROR "gtk-doc not found")
endif ()

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html DESTINATION share/gtk-doc/html/libhif)