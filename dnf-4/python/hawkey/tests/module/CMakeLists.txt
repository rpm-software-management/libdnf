if(${PYTHON_VERSION_MAJOR} STREQUAL "2")
    set(TEST_MODULE_NAME "_hawkey_testmodule")
else()
    set(TEST_MODULE_NAME "_hawkey_test")
endif()

find_package(PythonInstDir)
include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${CMAKE_SOURCE_DIR})
link_directories("${CMAKE_SOURCE_DIR}/tests/hawkey")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

set(testmodule_SRCS
    hawkey_testmodule.cpp
)

add_library(testmodule SHARED ${testmodule_SRCS})
set_target_properties(testmodule PROPERTIES PREFIX "")
set_target_properties(testmodule PROPERTIES OUTPUT_NAME ${TEST_MODULE_NAME})
target_link_libraries(testmodule _hawkeymodule
    testshared
    ${PYTHON_LIBRARY}
)

# copy the whole package under hawkey/test so 'import hawkey.test' works as expected:
set(pytest_COPIES __init__.py)
set(TMODULE_TARGET ${CMAKE_BINARY_DIR}/python/hawkey/hawkey/test)
file(COPY ${pytest_COPIES} DESTINATION ${TMODULE_TARGET})
add_custom_command(TARGET testmodule
    POST_BUILD
    COMMAND cp $<TARGET_FILE:testmodule> ${TMODULE_TARGET}
)

install(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/hawkey/test)
install(TARGETS testmodule LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/hawkey/test)
