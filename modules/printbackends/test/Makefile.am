include $(top_srcdir)/Makefile.decl

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

backenddir = $(libdir)/btk-2.0/$(BTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-test.la

libprintbackend_test_la_SOURCES = \
	btkprintbackendtest.c

libprintbackend_test_la_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/btk				\
	-I$(top_builddir)/btk				\
	-I$(top_srcdir)/bdk				\
	-I$(top_builddir)/bdk				\
	-DBTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(AM_CPPFLAGS)

libprintbackend_test_la_CFLAGS = \
	$(BTK_DEP_CFLAGS)				\
	$(AM_CFLAGS)

libprintbackend_test_la_LDFLAGS =  \
	-avoid-version -module $(no_undefined)

libprintbackend_test_la_LIBADD = \
	$(top_builddir)/btk/$(btktargetlib)		\
	$(BTK_DEP_LIBS)

noinst_HEADERS =			\
	btkprintbackendtest.h

-include $(top_srcdir)/git.mk
