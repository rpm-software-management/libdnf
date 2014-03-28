if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)
endif

SHARED_CPPFLAGS =						\
	$(GLIB_CFLAGS)						\
	$(HAWKEY_CFLAGS)					\
	-I$(top_srcdir)/libhif					\
	-I$(top_builddir)/libhif				\
	-I.							\
	-DHIF_COMPILATION					\
	-DTESTDATADIR=\""$(top_srcdir)/data/tests"\"		\
	-DG_LOG_DOMAIN=\"Hif\"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = 						\
	libhif.pc

hif-resources.c: libhif.gresource.xml
	$(AM_V_GEN)						\
	glib-compile-resources					\
		--sourcedir=$(srcdir)				\
		--sourcedir=$(top_builddir)/data		\
		--target=$@					\
		--generate-source				\
		--c-name hif					\
		$(srcdir)/libhif.gresource.xml
hif-resources.h: libhif.gresource.xml
	$(AM_V_GEN)						\
	glib-compile-resources					\
		--sourcedir=$(srcdir)				\
		--sourcedir=$(top_builddir)/data		\
		--target=$@					\
		--generate-header				\
		--c-name hif					\
		$(srcdir)/libhif.gresource.xml

BUILT_SOURCES =							\
	hif-resources.c						\
	hif-resources.h

lib_LTLIBRARIES =						\
	libhif.la

libhif_includedir = $(includedir)/libhif
libhif_include_HEADERS =					\
	libhif.h						\
	hif-version.h

libhif_la_SOURCES =						\
	hif-lock.c						\
	hif-lock.h						\
	hif-source.c						\
	hif-source.h						\
	hif-state.c						\
	hif-state.h						\
	hif-utils.c						\
	hif-utils.h						\
	hif-version.h

CLEANFILES = $(BUILT_SOURCES)

libhif_la_LIBADD =						\
	$(HAWKEY_LIBS)						\
	$(GLIB_LIBS)

libhif_la_LDFLAGS =						\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-export-dynamic						\
	-no-undefined						\
	-export-symbols-regex '^hif_.*'

libhif_la_CFLAGS =						\
	$(WARNINGFLAGS_C)
libhif_la_CPPFLAGS =						\
	$(SHARED_CPPFLAGS)					\
	-DPIDFILE=\""/var/run/hif"\"

check_PROGRAMS =						\
	hif-self-test
hif_self_test_SOURCES =						\
	hif-lock.c						\
	hif-source.c						\
	hif-state.c						\
	hif-utils.c						\
	hif-self-test.c
hif_self_test_LDADD =						\
	$(HAWKEY_LIBS)						\
	$(GLIB_LIBS)
hif_self_test_CFLAGS = $(WARNINGFLAGS_C)
hif_self_test_CPPFLAGS =					\
	$(SHARED_CPPFLAGS)					\
	-DPIDFILE=\""/tmp/hif"\"

TESTS = hif-self-test

if HAVE_INTROSPECTION
introspection_sources =						\
	hif-source.c						\
	hif-source.h						\
	hif-version.h

Hif-1.0.gir: libhif.la
Hif_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
Hif_1_0_gir_CFLAGS = $(SHARED_CPPFLAGS)
Hif_1_0_gir_SCANNERFLAGS = --identifier-prefix=Hif \
				--symbol-prefix=hif_ \
				--warn-all \
				--add-include-path=$(srcdir)
Hif_1_0_gir_EXPORT_PACKAGES = libhif
Hif_1_0_gir_LIBS = libhif.la
Hif_1_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += Hif-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

DISTCLEANFILES =						\
	*.log							\
	*.trs

EXTRA_DIST =							\
	libhif.gresource.xml					\
	libhif.pc.in						\
	hif-version.h.in

clean-local:
	rm -f *~
	rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
