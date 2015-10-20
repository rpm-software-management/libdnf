
SHARED_CPPFLAGS =						\
	$(GLIB_CFLAGS)						\
	$(CHECK_CFLAGS)						\
	-I$(top_srcdir)						\
	-I$(top_builddir)/libhif				\
	-DTESTDATADIR=\""$(top_srcdir)/data/tests/hawkey"\"	\
	-I.							\
	-DG_LOG_DOMAIN=\"Hif\"

check_PROGRAMS =						\
	hawkey-self-test
hawkey_self_test_SOURCES =					\
	fixtures.c						\
	fixtures.h						\
	test_advisory.c						\
	test_advisorypkg.c					\
	test_advisoryref.c					\
	test_goal.c						\
	test_iutil.c						\
	test_main.c						\
	test_package.c						\
	test_packagelist.c					\
	test_packageset.c					\
	test_query.c						\
	test_reldep.c						\
	test_repo.c						\
	test_sack.c						\
	test_selector.c						\
	testshared.c						\
	testshared.h						\
	test_subject.c						\
	test_suites.h						\
	testsys.c						\
	testsys.h						\
	test_util.c
hawkey_self_test_LDADD =					\
	$(top_builddir)/libhif/libhif.la			\
	$(CHECK_LIBS)						\
	$(GLIB_LIBS)
hawkey_self_test_CFLAGS = $(WARNINGFLAGS_C)
hawkey_self_test_CPPFLAGS =					\
	$(SHARED_CPPFLAGS)

TESTS = hawkey-self-test

-include $(top_srcdir)/git.mk
