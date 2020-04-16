SET(POLKIT_DIR /usr/share/polkit-1/actions)

install (
    FILES "org.rpm.dnf.v1.policy"
    DESTINATION ${POLKIT_DIR}
)
