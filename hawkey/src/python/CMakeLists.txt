FIND_PACKAGE (PythonLibs)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (hawkey_COPIES __init__.py queries.py dbg.py)
FILE(COPY ${hawkey_COPIES} DESTINATION ./hawkey/)

SET (hawkeymodule_SRCS
    hawkeymodule.c goal-py.c iutil-py.c package-py.c sack-py.c query-py.c repo-py.c)

ADD_LIBRARY(_hawkeymodule SHARED ${hawkeymodule_SRCS})
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(_hawkeymodule PROPERTIES LIBRARY_OUTPUT_DIRECTORY "./hawkey")
TARGET_LINK_LIBRARIES(_hawkeymodule libhawkey ${SOLV_LIBRARY} ${SOLVEXT_LIBRARY})
TARGET_LINK_LIBRARIES(_hawkeymodule ${PYTHON_LIBRARY})
