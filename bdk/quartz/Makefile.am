include $(top_srcdir)/Makefile.decl

libbdkincludedir = $(includedir)/btk-2.0/bdk

INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Bdk\"		\
	-DBDK_COMPILATION		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/bdk		\
	-I$(top_builddir)/bdk		\
	$(BTK_DEBUG_FLAGS) 		\
	$(BDK_DEP_CFLAGS)		\
	"-xobjective-c"

LDADDS = $(BDK_DEP_LIBS)

noinst_LTLIBRARIES = libbdk-quartz.la

libbdk_quartz_la_SOURCES =    	\
	BdkQuartzView.c		\
	BdkQuartzView.h		\
	BdkQuartzWindow.c	\
	BdkQuartzWindow.h	\
	bdkapplaunchcontext-quartz.c \
	bdkcolor-quartz.c	\
	bdkcursor-quartz.c	\
	bdkdisplay-quartz.c	\
	bdkdnd-quartz.c		\
	bdkdrawable-quartz.c	\
	bdkdrawable-quartz.h	\
	bdkevents-quartz.c	\
	bdkeventloop-quartz.c	\
	bdkfont-quartz.c	\
	bdkgc-quartz.c		\
	bdkgeometry-quartz.c	\
	bdkglobals-quartz.c	\
	bdkim-quartz.c		\
	bdkimage-quartz.c	\
	bdkinput.c		\
	bdkinputprivate.h	\
	bdkkeys-quartz.c	\
	bdkmain-quartz.c	\
	bdkpixmap-quartz.c	\
	bdkpixmap-quartz.h	\
	bdkprivate-quartz.h	\
	bdkproperty-quartz.c	\
	bdkquartz.h		\
	bdkscreen-quartz.c	\
	bdkscreen-quartz.h	\
	bdkselection-quartz.c	\
	bdkspawn-quartz.c	\
	bdktestutils-quartz.c	\
	bdkvisual-quartz.c	\
	bdkwindow-quartz.c	\
	bdkwindow-quartz.h	\
	xcursors.h

libbdkinclude_HEADERS = 	\
	bdkquartz.h


-include $(top_srcdir)/git.mk
