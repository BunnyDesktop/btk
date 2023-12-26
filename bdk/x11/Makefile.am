## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

libbdkincludedir = $(includedir)/btk-2.0/bdk

INCLUDES = 			\
	-DG_LOG_DOMAIN=\"Bdk\"	\
	-DBDK_COMPILATION	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/bdk	\
	-I$(top_builddir)/bdk	\
	$(BTK_DEBUG_FLAGS) 	\
	$(BDK_DEP_CFLAGS)

LDADDS = $(BDK_DEP_LIBS)

noinst_LTLIBRARIES = libbdk-x11.la

libbdk_x11_la_SOURCES =    	\
	MwmUtil.h	   	\
	bdkapplaunchcontext-x11.c \
	bdkasync.c		\
	bdkasync.h		\
	bdkcolor-x11.c	   	\
	bdkcursor-x11.c	   	\
	bdkdisplay-x11.c	\
	bdkdisplay-x11.h	\
	bdkdnd-x11.c	   	\
	bdkdrawable-x11.c  	\
	bdkdrawable-x11.h	\
	bdkevents-x11.c	   	\
	bdkfont-x11.c	   	\
	bdkgc-x11.c 	   	\
	bdkgeometry-x11.c  	\
	bdkglobals-x11.c   	\
	bdkim-x11.c	   	\
	bdkimage-x11.c	   	\
	bdkinput.c	   	\
	bdkkeys-x11.c		\
	bdkmain-x11.c	   	\
	bdkpixmap-x11.c	   	\
	bdkpixmap-x11.h		\
	bdkproperty-x11.c  	\
	bdkscreen-x11.c		\
	bdkscreen-x11.h		\
	bdkselection-x11.c 	\
	bdkspawn-x11.c		\
	bdktestutils-x11.c	\
	bdkvisual-x11.c	   	\
	bdkwindow-x11.c    	\
	bdkwindow-x11.h		\
	bdkxftdefaults.c	\
	bdkxid.c	   	\
	bdkx.h		   	\
	bdkprivate-x11.h   	\
	bdkinputprivate.h  	\
	xsettings-client.h	\
	xsettings-client.c	\
	xsettings-common.h	\
	xsettings-common.c

if XINPUT_XFREE
libbdk_x11_la_SOURCES += bdkinput-x11.c bdkinput-xfree.c
else
libbdk_x11_la_SOURCES += bdkinput-none.c
endif


libbdkinclude_HEADERS = 	\
	bdkx.h


noinst_PROGRAMS = checksettings
checksettings_LDADD = libbdk-x11.la $(BUNNYLIB_LIBS)

TESTS = checksettings

# We need to include all these C files here since the conditionals
# don't seem to be correctly expanded for the dist files.
EXTRA_DIST += 			\
	bdkinput-x11.c		\
	bdkinput-xfree.c	\
	bdkinput-none.c		\
	bdksettings.c

-include $(top_srcdir)/git.mk
