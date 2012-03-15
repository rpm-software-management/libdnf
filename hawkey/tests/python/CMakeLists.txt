FIND_PACKAGE (PythonLibs)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (testmodule_SRCS
     hawkey_testmodule.c)

ADD_LIBRARY(testmodule SHARED ${testmodule_SRCS})
SET_TARGET_PROPERTIES(testmodule PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(testmodule PROPERTIES OUTPUT_NAME "_hawkey_testmodule")
TARGET_LINK_LIBRARIES(testmodule _hawkeymodule
				 testsys
				 ${CHECK_LIBRARY}
				 ${PYTHON_LIBRARY})

# copy the whole package under hawkey/test so 'import hawkey.test' works as expected:
SET (pytest_COPIES __init__.py)
SET (TMODULE_TARGET ${CMAKE_BINARY_DIR}/src/python/hawkey/test)
FILE(COPY ${pytest_COPIES} DESTINATION ${TMODULE_TARGET})
ADD_CUSTOM_COMMAND(TARGET testmodule
		   POST_BUILD
		   COMMAND cp $<TARGET_FILE:testmodule> ${TMODULE_TARGET})

ADD_TEST(test_python /usr/bin/python "${CMAKE_CURRENT_SOURCE_DIR}/test_sack.py"
		     "${CMAKE_BINARY_DIR}/src/python"
		     "${CMAKE_SOURCE_DIR}/tests/repos/")
