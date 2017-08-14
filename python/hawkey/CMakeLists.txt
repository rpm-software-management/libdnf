if (${PYTHON_DESIRED} STREQUAL "2")
    message("Building for python2")
    FIND_PACKAGE (PythonInterp 2 REQUIRED)
    FIND_PACKAGE (PythonLibs 2 REQUIRED)
    SET(HAWKEY_MODULE_NAME "_hawkeymodule")
else()
    message("Building for python3")
    FIND_PACKAGE(PythonInterp 3 REQUIRED)
    FIND_PACKAGE(PythonLibs 3 REQUIRED)
    SET(HAWKEY_MODULE_NAME "_hawkey")
endif()

FIND_PACKAGE(PythonInstDir)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})
INCLUDE_DIRECTORIES ("${CMAKE_SOURCE_DIR}/libdnf")

MESSAGE(STATUS "Python install dir is ${PYTHON_INSTALL_DIR}")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (hawkey_COPIES __init__.py dbg.py)
FILE(COPY ${hawkey_COPIES} DESTINATION ./hawkey/)

SET (hawkeymodule_SRCS
    advisory-py.c
    advisorypkg-py.c
    advisoryref-py.c
    exception-py.c
    goal-py.c
    hawkeymodule.c
    iutil-py.c
    nevra-py.c
    module-form-py.c
    package-py.c
    packagedelta-py.c
    possibilities-py.c
    query-py.c
    repo-py.c
    reldep-py.c
    sack-py.c
    selector-py.c
    solution-py.c
    subject-py.c
    pycomp.c)

ADD_LIBRARY(_hawkeymodule SHARED ${hawkeymodule_SRCS})
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES OUTPUT_NAME ${HAWKEY_MODULE_NAME})
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES LIBRARY_OUTPUT_DIRECTORY "./hawkey")
TARGET_LINK_LIBRARIES(_hawkeymodule libdnf ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(_hawkeymodule ${PYTHON_LIBRARY})

INSTALL(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/hawkey)
INSTALL(TARGETS _hawkeymodule LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/hawkey)

ADD_SUBDIRECTORY(tests)
