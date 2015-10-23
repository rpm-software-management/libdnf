
hawkeydir = $(pythondir)/hawkey
hawkey_PYTHON = __init__.py

pyexec_LTLIBRARIES = _hawkey.la

_hawkey_la_SOURCES =						\
	advisorypkg-py.c					\
	advisorypkg-py.h					\
	advisory-py.c						\
	advisory-py.h						\
	advisoryref-py.c					\
	advisoryref-py.h					\
	exception-py.c						\
	exception-py.h						\
	goal-py.c						\
	goal-py.h						\
	hawkeymodule.c						\
	hawkey-pysys.h						\
	iutil-py.c						\
	iutil-py.h						\
	nevra-py.c						\
	nevra-py.h						\
	packagedelta-py.c					\
	packagedelta-py.h					\
	package-py.c						\
	package-py.h						\
	possibilities-py.c					\
	possibilities-py.h					\
	pycomp.c						\
	pycomp.h						\
	query-py.c						\
	query-py.h						\
	reldep-py.c						\
	reldep-py.h						\
	repo-py.c						\
	repo-py.h						\
	sack-py.c						\
	sack-py.h						\
	selector-py.c						\
	selector-py.h						\
	subject-py.c						\
	subject-py.h
_hawkey_la_CPPFLAGS =						\
	$(PYTHON3_CFLAGS)					\
	$(GLIB_CFLAGS)						\
	-DHIF_COMPILATION					\
	-I$(top_srcdir)/libhif
_hawkey_la_LDFLAGS = -module -avoid-version
_hawkey_la_LIBADD = $(top_builddir)/libhif/libhif.la

EXTRA_DIST = __init__.py

-include $(top_srcdir)/git.mk
