## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

libbdkincludedir = $(includedir)/btk-2.0/bdk

INCLUDES = \
	-DG_LOG_DOMAIN=\"Bdk\"	\
	-DINSIDE_BDK_WIN32	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/bdk	\
	-I$(top_builddir)/bdk	\
	$(BTK_DEBUG_FLAGS) 	\
	$(BDK_DEP_CFLAGS)	\
	$(BDK_WIN32_EXTRA_CFLAGS)\
	-DBDK_COMPILATION

LDADDS = $(BDK_DEP_LIBS)

noinst_LTLIBRARIES = libbdk-win32.la

SUBDIRS=rc

EXTRA_DIST += \
	bdfcursor.c \
	makefile.msc

libbdk_win32_la_SOURCES = \
	xcursors.h \
	bdkapplaunchcontext-win32.c \
	bdkcolor-win32.c \
	bdkcursor-win32.c \
	bdkdisplay-win32.c \
	bdkdnd-win32.c \
	bdkdrawable-win32.c \
	bdkdrawable-win32.h \
	bdkevents-win32.c \
	bdkfont-win32.c \
	bdkgc-win32.c \
	bdkgeometry-win32.c \
	bdkglobals-win32.c \
	bdkim-win32.c \
	bdkimage-win32.c \
	bdkinput.c \
	bdkinput-win32.c \
	bdkinput-win32.h \
	bdkkeys-win32.c \
	bdkmain-win32.c \
	bdkpixmap-win32.c \
	bdkpixmap-win32.h \
	bdkprivate-win32.h \
	bdkproperty-win32.c \
	bdkscreen-win32.c \
	bdkselection-win32.c \
	bdkspawn-win32.c \
	bdktestutils-win32.c \
	bdkvisual-win32.c \
	bdkwin32.h \
	bdkwin32keys.h \
	bdkwin32id.c \
	bdkwindow-win32.c \
	bdkwindow-win32.h \
	wintab.h \
	pktdef.h

libbdkinclude_HEADERS =		\
	bdkwin32.h

# -------- MSVC Project Items -----
MSVCPROJS = bdk-win32

bdk_win32_FILES = $(libbdk_win32_la_SOURCES)
bdk_win32_EXCLUDES = bdkwin32dummy

bdk_win32_HEADERS_DIR = $(libbdkincludedir)
bdk_win32_HEADERS_INST = $(libbdkinclude_HEADERS)
bdk_win32_HEADERS_EXCLUDES = bdkwin32dummy

include $(top_srcdir)/build/Makefile.msvcproj

dist-hook: $(top_builddir)/build/win32/vs9/bdk-win32.vcproj $(top_builddir)/build/win32/vs9/bdk-win32.headers

-include $(top_srcdir)/git.mk
