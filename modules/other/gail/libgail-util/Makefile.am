include $(top_srcdir)/Makefile.decl

EXTRA_DIST += bailutil.def
if PLATFORM_WIN32
no_undefined = -no-undefined
endif
if OS_WIN32
export_symbols = -export-symbols $(srcdir)/bailutil.def
install-def-file: bailutil.def
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/bailutil.def $(DESTDIR)$(libdir)
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/bailutil.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = bailutil.lib

install-ms-lib:
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL) bailutil.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/bailutil.lib
else
install-ms-lib:
uninstall-ms-lib:
endif


lib_LTLIBRARIES = libbailutil.la

util_c_sources =		\
	bailmisc.c		\
	bailtextutil.c

libbailutilincludedir=$(includedir)/bail-1.0/libbail-util

util_public_h_sources =		\
	bailmisc.h		\
	bailtextutil.h		\
	bail-util.h

libbailutil_la_SOURCES = 	\
	$(util_c_sources)

libbailutilinclude_HEADERS =	\
	$(util_public_h_sources)

libbailutil_la_CPPFLAGS = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/bdk	\
	-I$(top_builddir)/bdk	\
	-I$(top_srcdir)/btk	\
	-I$(top_builddir)/btk	\
	$(AM_CPPFLAGS) 		\
	-DBDK_DISABLE_DEPRECATED\
	-DBTK_DISABLE_DEPRECATED

libbailutil_la_CFLAGS = \
	$(BTK_DEP_CFLAGS)	\
	$(BTK_DEBUG_FLAGS)	\
	$(AM_CFLAGS)

libbailutil_la_LIBADD = \
	$(top_builddir)/btk/$(btktargetlib)	\
	$(top_builddir)/bdk/$(bdktargetlib)	\
	$(BTK_DEP_LIBS)

libbailutil_la_LDFLAGS = \
	-version-info $(BAIL_LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(export_symbols)			\
        $(LDFLAGS)

# ----------------- MSVC Projects ------------------
MSVCPROJS = bailutil

bailutil_FILES = $(libbailutil_la_SOURCES)
bailutil_EXCLUDES = baildummy

bailutil_HEADERS_DIR = $(libbailutilincludedir)
bailutil_HEADERS_INST = $(libbailutilinclude_HEADERS)
bailutil_HEADERS_EXCLUDES = baildummy

include $(top_srcdir)/build/Makefile.msvcproj

dist-hook: $(top_builddir)/build/win32/vs9/bailutil.vcproj $(top_builddir)/build/win32/vs9/bailutil.headers

bailutil.lib: libbailutil.la bailutil.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libbailutil-$(BAIL_LT_CURRENT_MINUS_AGE).dll -def:bailutil.def -out:$@

install-data-local: install-ms-lib install-def-file

uninstall-local: uninstall-ms-lib uninstall-def-file

-include $(top_srcdir)/git.mk
