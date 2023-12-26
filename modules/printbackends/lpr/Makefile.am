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
	-DBTK_PRINT_BACKEND_ENABLE_UNSUPPORTED		\
	$(BTK_DEP_CFLAGS)				\
	$(BTK_DEBUG_FLAGS)

LDADDS = \
	$(BTK_DEP_LIBS)					\
	$(top_builddir)/btk/$(btktargetlib)		

backenddir = $(libdir)/btk-2.0/$(BTK_BINARY_VERSION)/printbackends

backend_LTLIBRARIES = libprintbackend-lpr.la

libprintbackend_lpr_la_SOURCES =	\
	btkprintbackendlpr.c

noinst_HEADERS =			\
	btkprintbackendlpr.h

libprintbackend_lpr_la_LDFLAGS =  -avoid-version -module $(no_undefined)
libprintbackend_lpr_la_LIBADD = $(LDADDS)

-include $(top_srcdir)/git.mk
