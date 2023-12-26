## Makefile.am for btk+/bdk/bdk-directfb
include $(top_srcdir)/Makefile.decl

libbdkincludedir = $(includedir)/btk-2.0/bdk

INCLUDES = 		\
	-DG_LOG_DOMAIN=\"Bdk-DirectFB\"	\
	-DBDK_COMPILATION \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/bdk		\
	-I$(top_builddir)/bdk		\
	-DG_DISABLE_DEPRECATED		\
	@BTK_DEBUG_FLAGS@		\
	@BDK_DEP_CFLAGS@		

LDADDS = 		\
	@BDK_DEP_LIBS@		

noinst_LTLIBRARIES = libbdk-directfb.la

libbdk_directfb_la_SOURCES =	\
	bdkapplaunchcontext-directfb.c \
	bdkcolor-directfb.c	\
	bdkcursor-directfb.c	\
	bdkdnd-directfb.c	\
	bdkdisplay-directfb.c   \
	bdkdisplay-directfb.h   \
	bdkdrawable-directfb.c	\
	bdkevents-directfb.c	\
	bdkfont-directfb.c	\
	bdkgc-directfb.c	\
	bdkgeometry-directfb.c	\
	bdkglobals-directfb.c	\
	bdkim-directfb.c	\
	bdkimage-directfb.c	\
	bdkinput-directfb.c	\
	bdkinput-directfb.h	\
	bdkkeys-directfb.c	\
	bdkmain-directfb.c	\
	bdkpixmap-directfb.c	\
	bdkprivate-directfb.h	\
	bdkproperty-directfb.c	\
	bdkscreen-directfb.c	\
	bdkselection-directfb.c	\
	bdkspawn-directfb.c 	\
	bdktestutils-directfb.c	\
	bdkvisual-directfb.c	\
	bdkdirectfb.h		\
	bdkwindow-directfb.c	\
	bdkwindowid.c		\
	x-cursors.xbm

libbdkinclude_HEADERS =		\
	bdkdirectfb.h 		\
	bdkprivate-directfb.h


EXTRA_DIST += AUTHORS README TODO 

-include $(top_srcdir)/git.mk
