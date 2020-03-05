if(NOT WITH_LIBDNF_CLI)
    return()
endif()

# use any tests found under the current directory
file(GLOB_RECURSE RUBY_TESTS *.rb)

foreach(FILE ${RUBY_TESTS})
    add_ruby_test(libdnf_cli "${FILE}")
endforeach()
