message("Building tests")


# libdnf
add_subdirectory(libdnf)
add_subdirectory(libdnf-cli)

# bindings
add_subdirectory(go)
add_subdirectory(perl5)
add_subdirectory(python3)
add_subdirectory(ruby)
