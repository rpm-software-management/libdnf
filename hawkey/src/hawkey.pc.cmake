prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@CMAKE_INSTALL_PREFIX@/include

Name: hawkey
Description: Resolves dependencies based on RPMDB and yum repos.
Version: @VERSION@
Requires: glib-2.0
Requires.private: rpm zlib
Libs: -L${libdir} -lhawkey -lsolv -lsolvext
Libs.private: -lexpat
Cflags: -I${includedir} -D_FILE_OFFSET_BITS=64
