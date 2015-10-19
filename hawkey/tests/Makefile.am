
SHARED_CPPFLAGS =						\
	$(GLIB_CFLAGS)						\
	-I$(top_srcdir)/libhif					\
	-I$(top_builddir)/libhif				\
	-I.							\
	-DG_LOG_DOMAIN=\"Hif\"

check_PROGRAMS =						\
	hawkey-self-test
hawkey_self_test_SOURCES =					\
	$(libhawkey_la_SOURCES)					\
	fixtures.c						\
	test_advisory.c						\
	test_advisorypkg.c					\
	test_advisoryref.c					\
	test_goal.c						\
	test_iutil.c						\
	test_main.c						\
	test_package.c						\
	test_packagelist.c					\
	test_packageset.c					\
	test_reldep.c						\
	test_repo.c						\
	test_query.c						\
	test_sack.c						\
	test_selector.c						\
	test_subject.c						\
	test_util.c						\
	testshared.c						\
	testsys.c)
hawkey_self_test_LDADD =					\
	../libhif.la						\
	$(GLIB_LIBS)
hawkey_self_test_CFLAGS = $(WARNINGFLAGS_C)
hawkey_self_test_CPPFLAGS =					\
	$(SHARED_CPPFLAGS)

TESTS = hawkey-self-test

-include $(top_srcdir)/git.mk
