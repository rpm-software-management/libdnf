AC_PREREQ(2.63)

m4_define([hif_major_version], [0])
m4_define([hif_minor_version], [1])
m4_define([hif_micro_version], [8])
m4_define([hif_version],
          [hif_major_version.hif_minor_version.hif_micro_version])

AC_INIT([libhif],[hif_version])
AC_CONFIG_SRCDIR(libhif)
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-xz tar-ustar foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_SYS_LARGEFILE

GOBJECT_INTROSPECTION_CHECK([0.9.8])

HIF_MAJOR_VERSION=hif_major_version
HIF_MINOR_VERSION=hif_minor_version
HIF_MICRO_VERSION=hif_micro_version
HIF_VERSION=hif_version
AC_SUBST(HIF_MAJOR_VERSION)
AC_SUBST(HIF_MINOR_VERSION)
AC_SUBST(HIF_MICRO_VERSION)
AC_SUBST(HIF_VERSION)

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# libtool versioning - this applies to libpackagekit
#
# See http://sources.redhat.com/autobook/autobook/autobook_91.html#SEC91 for details
#
# - If interfaces have been changed or added, but binary compatibility
#   has been preserved, change:
#      CURRENT += 1
#      REVISION = 0
#      AGE += 1
# - If binary compatibility has been broken (eg removed or changed
#   interfaces), change:
#      CURRENT += 1
#      REVISION = 0
#      AGE = 0
# - If the interface is the same as the previous version, but bugs are
#   fixed, change:
#      REVISION += 1
LT_CURRENT=1
LT_REVISION=6
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_HEADER_STDC
LT_INIT
AM_PROG_CC_C_O

# set up gtk-doc
GTK_DOC_CHECK(1.9)

CC_CHECK_FLAGS_APPEND([WARNINGFLAGS_C], [CFLAGS], [\
	-Wall \
	-Wcast-align -Wno-uninitialized \
	-Wmissing-declarations \
	-Wredundant-decls \
	-Wpointer-arith \
	-Wcast-align \
	-Wwrite-strings \
	-Winit-self \
	-Wreturn-type \
	-Wformat-nonliteral \
	-Wformat-security \
	-Wmissing-include-dirs \
	-Wmissing-format-attribute \
	-Wsign-compare \
	-Wtype-limits \
	-Wuninitialized \
	-Waggregate-return \
	-Wdeclaration-after-statement \
	-Wshadow \
	-Winline \
	-fno-strict-aliasing \
])
AC_SUBST(WARNINGFLAGS_C)

PKG_CHECK_MODULES(GLIB, gio-unix-2.0 >= 2.36.0)
GLIB_CFLAGS="$GLIB_CFLAGS -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_36 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_36"
PKG_CHECK_MODULES(HAWKEY, hawkey >= 0.4.6 librepo >= 1.7.11 rpm >= 4.11.0)

AC_CONFIG_FILES([
Makefile
libhif/Makefile
libhif/libhif.pc
libhif/hif-version.h
data/Makefile
data/tests/Makefile
docs/Makefile
docs/api/Makefile
docs/api/version.xml
])
AC_OUTPUT
