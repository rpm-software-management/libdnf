message("Building tests")


add_subdirectory(libdnf)


if(WITH_GO)
    add_subdirectory(go)
endif()


if(WITH_PERL5)
    add_subdirectory(perl5)
endif()


if(WITH_PYTHON3)
    add_subdirectory(python3)
endif()


if(WITH_RUBY)
    add_subdirectory(ruby)
endif()
