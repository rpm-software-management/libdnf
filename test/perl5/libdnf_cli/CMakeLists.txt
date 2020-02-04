if(NOT WITH_LIBDNF_CLI)
    return()
endif()

# List tests explicitly. Globbing is unreliable according to the CMake
# documention.
# We need to define the Perl tests as CMake tests to achieve parallelism.
foreach(FILE
        progressbar/load.t
        )
    add_perl_test(libdnf_cli "${FILE}")
endforeach()
