AC_PREREQ(2.63)

m4_define([hif_major_version], [0])
m4_define([hif_minor_version], [3])
m4_define([hif_micro_version], [0])
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
LT_CURRENT=2
LT_REVISION=0
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
	-Wcast-align \
	-Wwrite-strings \
	-Wreturn-type \
	-Wformat-nonliteral \
	-Wmissing-format-attribute \
	-Wsign-compare \
	-Wtype-limits \
	-Wuninitialized \
	-Waggregate-return \
	-Wshadow \
	-Winline \
	-fno-strict-aliasing \
])
AC_SUBST(WARNINGFLAGS_C)

PKG_CHECK_MODULES(GLIB, gio-unix-2.0 >= 2.36.0)
GLIB_CFLAGS="$GLIB_CFLAGS -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_36 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_36"
PKG_CHECK_MODULES(HAWKEY, libsolv librepo >= 1.7.11 rpm >= 4.11.0)
PKG_CHECK_MODULES(PYTHON3, python3)
PKG_CHECK_MODULES(CHECK, check)

AM_PATH_PYTHON(3.0.0)
PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`

# Hawkey ABI change in 0.5.3
AC_MSG_CHECKING([For hawkey 0.5.3 ABI break])
AC_TRY_COMPILE([#include <stdlib.h>
#include <hawkey/sack.h>],
               [hy_sack_create(NULL, NULL, NULL, 0); return 0;],
               [AC_MSG_RESULT([no]); BUILDOPT_HAWKEY_SACK_CREATE2=0],
               [AC_MSG_RESULT([yes]); BUILDOPT_HAWKEY_SACK_CREATE2=1])
AC_DEFINE_UNQUOTED(BUILDOPT_HAWKEY_SACK_CREATE2, $BUILDOPT_HAWKEY_SACK_CREATE2, [Hawkey ABI change])
AC_PATH_PROG([XSLTPROC], [xsltproc])

# whether to use /var/lib/dnf/yumdb or /var/lib/yum/yumdb
AC_ARG_ENABLE(dnf-yumdb, AS_HELP_STRING([--enable-dnf-yumdb],[use dnf/yumdb instead of yum/yumdb @<:@default=yes@:>@]),
              enable_dnf_yumdb=$enableval,enable_dnf_yumdb=yes)
if test x$enable_dnf_yumdb = xyes; then
  AC_DEFINE_UNQUOTED(BUILDOPT_USE_DNF_YUMDB,1,[Use dnf/yumdb instead of yum/yumdb])
fi

AC_CONFIG_FILES([
Makefile
python/Makefile
python/hawkey/Makefile
libhif/Makefile
libhif/libhif.pc
libhif/hif-version.h
tests/Makefile
tests/hawkey/Makefile
data/Makefile
data/tests/Makefile
docs/Makefile
docs/api/Makefile
docs/api/version.xml
])
AC_OUTPUT
