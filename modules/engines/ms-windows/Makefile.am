include $(top_srcdir)/Makefile.decl

SUBDIRS=Theme

EXTRA_DIST += Makefile.msc

INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/bdk		\
	-I$(top_builddir)/bdk		\
	-DBDK_DISABLE_DEPRECATED	\
	$(BTK_DEP_CFLAGS)	\
	$(BMODULE_CFLAGS)


LDADDS = \
	$(top_builddir)/bdk/$(bdktargetlib)		\
	$(top_builddir)/btk/$(btktargetlib)		\
	$(BTK_DEP_LIBS)		\
	$(BMODULE_LIBS)

enginedir = $(libdir)/btk-2.0/$(BTK_BINARY_VERSION)/engines

engine_LTLIBRARIES = libwimp.la

libwimp_la_SOURCES = \
	msw_rc_style.c		\
	msw_rc_style.h		\
	msw_style.c		\
	msw_style.h		\
	msw_theme_main.c	\
	xp_theme.c		\
	xp_theme_defs.h		\
	xp_theme.h

libwimp_la_LDFLAGS = \
	-avoid-version -module -no-undefined -export-dynamic

libwimp_la_LIBADD = $(LDADDS) -lgdi32

# ---------------- MSVC Projects ---------------
MSVCPROJS = libwimp

libwimp_FILES = $(libwimp_la_SOURCES)
libwimp_EXCLUDES = libwimpdummy

include $(top_srcdir)/build/Makefile.msvcproj

dist-hook: $(top_builddir)/build/win32/vs9/libwimp.vcproj

-include $(top_srcdir)/git.mk
