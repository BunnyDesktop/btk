include $(top_srcdir)/Makefile.decl

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

backenddir = $(libdir)/btk-2.0/$(BTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-file.la

libprintbackend_file_la_SOURCES = \
	btkprintbackendfile.c

libprintbackend_file_la_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/btk				\
	-I$(top_builddir)/btk				\
	-I$(top_srcdir)/bdk				\
	-I$(top_builddir)/bdk				\
	-DBTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(AM_CPPFLAGS)

libprintbackend_file_la_CFLAGS = \
	$(BTK_DEP_CFLAGS)				\
	$(AM_CFLAGS)

libprintbackend_file_la_LDFLAGS =  \
	-avoid-version -module $(no_undefined)

libprintbackend_file_la_LIBADD = \
	$(top_builddir)/btk/$(btktargetlib)		\
	$(top_builddir)/bdk/$(bdktargetlib)		\
	$(BTK_DEP_LIBS)

noinst_HEADERS =			\
	btkprintbackendfile.h

-include $(top_srcdir)/git.mk
