find_package(SWIG REQUIRED)
include(UseSWIG)


message("Building for perl")
find_package(Perl REQUIRED)
find_package(PerlLibs REQUIRED)


include_directories(${CMAKE_SOURCE_DIR})
include_directories(${PERL_INCLUDE_PATH})
include(UseSWIG)


set_source_files_properties(../swig/utils.i PROPERTIES CPLUSPLUS ON)
swig_add_module(utils perl ../swig/utils.i)
swig_link_libraries(utils libdnf)
swig_link_libraries(utils ${PERL_LIBRARY})
swig_link_libraries(utils ${SQLite3_LIBRARIES})

set_source_files_properties(../swig/transaction.i PROPERTIES CPLUSPLUS ON)
swig_add_module(transaction perl ../swig/transaction.i)
swig_link_libraries(transaction libdnf)
swig_link_libraries(transaction ${PERL_LIBRARY})
