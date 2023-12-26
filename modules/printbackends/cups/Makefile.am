include $(top_srcdir)/Makefile.decl

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/btk				\
	-I$(top_builddir)/btk				\
	-I$(top_srcdir)/bdk				\
	-I$(top_builddir)/bdk				\
	$(CUPS_CFLAGS)					\
	-DBTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(BTK_DEP_CFLAGS)				\
	$(BTK_DEBUG_FLAGS)

LDADDS = \
	$(top_builddir)/btk/$(btktargetlib)		\
	$(top_builddir)/bdk/$(bdktargetlib)		\
	$(BTK_DEP_LIBS)

backenddir = $(libdir)/btk-2.0/$(BTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-cups.la

libprintbackend_cups_la_SOURCES =	\
	btkprintbackendcups.c		\
	btkprintercups.c		\
	btkcupsutils.c

noinst_HEADERS =			\
	btkprintbackendcups.h		\
	btkprintercups.h		\
	btkcupsutils.h

libprintbackend_cups_la_LDFLAGS =  -avoid-version -module $(no_undefined)
libprintbackend_cups_la_LIBADD = $(LDADDS) $(CUPS_LIBS)

-include $(top_srcdir)/git.mk
