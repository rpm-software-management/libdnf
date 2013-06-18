FIND_PACKAGE (PythonLibs 2)
FIND_PACKAGE (PythonInterp 2 REQUIRED)
FIND_PACKAGE (PythonInstDir 2 REQUIRED)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

MESSAGE(STATUS "Python install dir is ${PYTHON_INSTALL_DIR}")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (hawkey_COPIES __init__.py dbg.py)
FILE(COPY ${hawkey_COPIES} DESTINATION ./hawkey/)

SET (hawkeymodule_SRCS
    exception-py.c
    goal-py.c
    hawkeymodule.c
    iutil-py.c
    package-py.c
    packagedelta-py.c
    query-py.c
    repo-py.c
    reldep-py.c
    sack-py.c
    selector-py.c)

ADD_LIBRARY(_hawkeymodule SHARED ${hawkeymodule_SRCS})
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES LIBRARY_OUTPUT_DIRECTORY "./hawkey")
TARGET_LINK_LIBRARIES(_hawkeymodule libhawkey ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(_hawkeymodule ${PYTHON_LIBRARY})

INSTALL(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/hawkey)
INSTALL(TARGETS _hawkeymodule LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/hawkey)
