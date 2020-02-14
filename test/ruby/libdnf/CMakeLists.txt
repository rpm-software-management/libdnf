# List tests explicitly.
foreach(FILE
        utils/sack_test.rb
        utils/utils_test.rb
        )
    add_ruby_test(libdnf "${FILE}")
endforeach()
