find_package(PythonInstDir)
find_package(SWIG REQUIRED)
include(UseSWIG)

message(STATUS "Building for python${PYTHON_VERSION_MAJOR}")

find_package(PythonInstDir)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${PYTHON_INCLUDE_PATH})
include(UseSWIG)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # using 64bit architecture
    set(CMAKE_SWIG_FLAGS "-DSWIGWORDSIZE64")
endif()

# The libdnf API is under development now. This enables it for internal usage.
set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} "-DLIBDNF_UNSTABLE_API")

set(COMMON_TYPES_NAME "common_types")
set_source_files_properties(../swig/${COMMON_TYPES_NAME}.i PROPERTIES CPLUSPLUS ON)
swig_add_module(${COMMON_TYPES_NAME} python ../swig/${COMMON_TYPES_NAME}.i)
swig_link_libraries(${COMMON_TYPES_NAME} libdnf)
swig_link_libraries(${COMMON_TYPES_NAME} ${PYTHON_LIBRARY})
install(TARGETS _${COMMON_TYPES_NAME} LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/${COMMON_TYPES_NAME}.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set(CONFIG_NAME "conf")
set_source_files_properties(../swig/${CONFIG_NAME}.i PROPERTIES CPLUSPLUS ON)
swig_add_module(${CONFIG_NAME} python ../swig/${CONFIG_NAME}.i)
swig_link_libraries(${CONFIG_NAME} libdnf)
swig_link_libraries(${CONFIG_NAME} ${PYTHON_LIBRARY})
install(TARGETS _${CONFIG_NAME} LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/${CONFIG_NAME}.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set(REPO_NAME "repo")
set_source_files_properties(../swig/${REPO_NAME}.i PROPERTIES CPLUSPLUS ON)
swig_add_module(${REPO_NAME} python ../swig/${REPO_NAME}.i)
swig_link_libraries(${REPO_NAME} libdnf)
swig_link_libraries(${REPO_NAME} ${PYTHON_LIBRARY})
install(TARGETS _${REPO_NAME} LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/${REPO_NAME}.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set(SMARTCOLS_NAME "smartcols")
set_source_files_properties(../swig/${SMARTCOLS_NAME}.i PROPERTIES CPLUSPLUS ON)
swig_add_module(${SMARTCOLS_NAME} python ../swig/${SMARTCOLS_NAME}.i)
swig_link_libraries(${SMARTCOLS_NAME} libdnf)
swig_link_libraries(${SMARTCOLS_NAME} ${PYTHON_LIBRARY})
install(TARGETS _${SMARTCOLS_NAME} DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/${SMARTCOLS_NAME}.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set_source_files_properties(../swig/utils.i PROPERTIES CPLUSPLUS ON)
swig_add_module(utils python ../swig/utils.i)
swig_link_libraries(utils libdnf)
swig_link_libraries(utils ${PYTHON_LIBRARY})
swig_link_libraries(utils ${SQLite3_LIBRARIES})
install(TARGETS _utils DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/utils.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set_source_files_properties(../swig/transaction.i PROPERTIES CPLUSPLUS ON)
swig_add_module(transaction python ../swig/transaction.i)
swig_link_libraries(transaction libdnf)
swig_link_libraries(transaction ${PYTHON_LIBRARY})
install(TARGETS _transaction DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/transaction.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

set(MODULE_NAME "module")
set_source_files_properties(../swig/${MODULE_NAME}.i PROPERTIES CPLUSPLUS ON)
swig_add_module(${MODULE_NAME} python ../swig/${MODULE_NAME}.i)
swig_link_libraries(${MODULE_NAME} libdnf)
swig_link_libraries(${MODULE_NAME} ${PYTHON_LIBRARY})
install(TARGETS _${MODULE_NAME} LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
install(FILES ${CMAKE_BINARY_DIR}/bindings/python/${MODULE_NAME}.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)

configure_file(__init__.py ${CMAKE_CURRENT_BINARY_DIR}/__init__.py COPYONLY)
install(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/libdnf)
