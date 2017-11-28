
MESSAGE("Looking for CppUnit")

SET(CPPUNIT_FOUND "NO")

FIND_PATH(CPPUNIT_INCLUDE_DIR cppunit/TestCase.h /usr/local/include /usr/include)

list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "lib")

list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".so")
IF(APPLE)
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".dylib")
ENDIF(APPLE)

FIND_LIBRARY(CPPUNIT_LIBRARY cppunit
        ${CPPUNIT_INCLUDE_DIR}/../lib
        /usr/local/lib
        /usr/local/lib64
        /usr/lib
        /usr/lib64
        /lib
        /lib64)

IF(CPPUNIT_INCLUDE_DIR)
    IF(CPPUNIT_LIBRARY)
        SET(CPPUNIT_FOUND "YES")
        SET(CPPUNIT_LIBRARIES ${CPPUNIT_LIBRARY} ${CMAKE_DL_LIBS})
        MESSAGE("Found CppUnit")
    ELSE (CPPUNIT_LIBRARY)
        IF (CPPUNIT_FIND_REQUIRED)
            MESSAGE(SEND_ERROR "Could not find library CppUnit.")
        ENDIF (CPPUNIT_FIND_REQUIRED)
    ENDIF(CPPUNIT_LIBRARY)
ELSE(CPPUNIT_INCLUDE_DIR)
    IF (CPPUNIT_FIND_REQUIRED)
        MESSAGE(SEND_ERROR "Could not find library CppUnit.")
    ENDIF(CPPUNIT_FIND_REQUIRED)
ENDIF(CPPUNIT_INCLUDE_DIR)
