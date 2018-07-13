ADD_SUBDIRECTORY(modulemd)
ADD_SUBDIRECTORY(state)

SET (MODULE_SOURCES
        ${MODULE_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/ModulePackageContainer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModulePackageMaker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModulePackage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PlatformModulePackage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ModuleSolver.cpp
        PARENT_SCOPE
        )