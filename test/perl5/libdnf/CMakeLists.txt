# List tests explicitly. Globbing is unreliable according to the CMake
# documention.
# We need to define the Perl tests as CMake tests to achieve parallelism.
foreach(FILE
        base/load.t
        utils/load.t
        utils/random_int32.t
        )
    add_perl_test(libdnf "${FILE}")
endforeach()
