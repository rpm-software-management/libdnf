if(${PYTHON_VERSION_MAJOR} STREQUAL "2")
    message("Building for python2")
    set(HAWKEY_MODULE_NAME "_hawkeymodule")
else()
    message("Building for python3")
    set(HAWKEY_MODULE_NAME "_hawkey")
endif()

find_package(PythonInstDir)
include_directories(${PYTHON_INCLUDE_PATH})
include_directories("${CMAKE_SOURCE_DIR}/libdnf")

message(STATUS "Python install dir is ${PYTHON_INSTALL_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

set(hawkey_COPIES __init__.py)
file(COPY ${hawkey_COPIES} DESTINATION ./hawkey/)

set(hawkeymodule_SRCS
    advisory-py.cpp
    advisorypkg-py.cpp
    advisoryref-py.cpp
    exception-py.cpp
    goal-py.cpp
    hawkeymodule.cpp
    iutil-py.cpp
    nsvcap-py.cpp
    nevra-py.cpp
    package-py.cpp
    packagedelta-py.cpp
    query-py.cpp
    reldep-py.cpp
    repo-py.cpp
    sack-py.cpp
    selector-py.cpp
    subject-py.cpp
    pycomp.cpp
)

add_library(_hawkeymodule SHARED ${hawkeymodule_SRCS})
set_target_properties(_hawkeymodule PROPERTIES PREFIX "")
set_target_properties(_hawkeymodule PROPERTIES OUTPUT_NAME ${HAWKEY_MODULE_NAME})
set_target_properties(_hawkeymodule PROPERTIES LIBRARY_OUTPUT_DIRECTORY "./hawkey")
target_link_libraries(_hawkeymodule libdnf ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
target_link_libraries(_hawkeymodule ${PYTHON_LIBRARY})

install(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/hawkey)
install(TARGETS _hawkeymodule LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/hawkey)

add_subdirectory(tests)
