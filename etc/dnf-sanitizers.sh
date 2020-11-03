function dnf {
    ASAN_OPTIONS=verify_asan_link_order=0 /usr/bin/dnf "$@"
}

export -f dnf
