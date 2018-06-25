SET (LIBDNF_TEST_SOURCES
        ${LIBDNF_TEST_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/PackageTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageRequiresTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageWithCommandlinePackageTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageWithRealRepoTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PackageInstantiable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DependencyTest.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DependencyContainerTest.cpp
        PARENT_SCOPE
        )

SET (LIBDNF_TEST_HEADERS
        ${LIBDNF_TEST_HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/PackageTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageRequiresTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageWithCommandlinePackageTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RpmPackageWithRealRepoTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DependencyTest.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DependencyContainerTest.hpp
        PARENT_SCOPE
        )