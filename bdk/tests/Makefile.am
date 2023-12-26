include $(top_srcdir)/Makefile.decl

NULL=

# check_PROGRAMS=check-bdk-bairo
check_PROGRAMS=
TESTS=$(check_PROGRAMS)
TESTS_ENVIRONMENT=BDK_PIXBUF_MODULE_FILE=$(top_builddir)/bdk-pixbuf/bdk-pixbuf.loaders

AM_CPPFLAGS=\
	$(BDK_DEP_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_builddir)/bdk \
	$(NULL)

check_bdk_bairo_SOURCES=\
	check-bdk-bairo.c \
	$(NULL)
check_bdk_bairo_LDADD=\
	$(BDK_DEP_LIBS) \
	$(top_builddir)/bdk-pixbuf/libbdk_pixbuf-$(BTK_API_VERSION).la \
	$(top_builddir)/bdk/libbdk-$(bdktarget)-$(BTK_API_VERSION).la \
	$(NULL)

CLEANFILES = \
	bairosurface.png	\
	bdksurface.png

-include $(top_srcdir)/git.mk
