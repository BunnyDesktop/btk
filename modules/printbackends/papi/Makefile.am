if OS_WIN32
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

backend_LTLIBRARIES = libprintbackend-papi.la

libprintbackend_papi_la_SOURCES =	\
	btkprinterpapi.c		\
	btkprintbackendpapi.c

noinst_HEADERS =			\
	btkprinterpapi.h		\
	btkprintbackendpapi.h

libprintbackend_papi_la_LDFLAGS =  -avoid-version -module $(no_undefined)
libprintbackend_papi_la_LIBADD = $(LDADDS) -lpapi

-include $(top_srcdir)/git.mk
