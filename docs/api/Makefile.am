## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=libhif

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
DOC_SOURCE_DIR=../../libhif

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB=$(top_srcdir)/libhif/hif-*.h
CFILE_GLOB=$(top_srcdir)/libhif/hif-*.c

# Header files to ignore when scanning.
IGNORE_HFILES =						\
	config.h

# Images to copy into HTML directory.
#HTML_IMAGES =						\
#	spec/pk-transactions-trusted.png

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =						\
	version.xml

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
AM_CPPFLAGS =						\
	$(GLIB_CFLAGS)					\
	-I$(top_builddir)/libhif			\
	-I$(top_srcdir)/libhif				\
	$(NULL)

AS_LIBS = $(top_builddir)/libhif/libhif.la

GTKDOC_LIBS =						\
	$(GLIB_LIBS)					\
	$(AS_LIBS)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

CLEANFILES +=						\
	*.txt						\
	*.types						\
	*.args						\
	*.hierarchy					\
	*.interfaces					\
	*.prerequisites					\
	*.signals					\
	*.stamp						\
	tmpl

# Other files to distribute
EXTRA_DIST += version.xml.in

-include $(top_srcdir)/git.mk
