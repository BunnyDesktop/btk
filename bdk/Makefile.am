## Makefile.am for btk+/bdk
include $(top_srcdir)/Makefile.decl
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = \
   --warn-all \
   --add-include-path=../bdk
INTROSPECTION_COMPILER_ARGS = \
   --includedir=$(srcdir) \
   --includedir=.

SUBDIRS = $(bdktarget) . tests
DIST_SUBDIRS = win32 x11 quartz directfb tests

CLEANFILES =

EXTRA_DIST +=			\
	keynames.txt		\
	keyname-table.h		\
	gen-keyname-table.pl	\
	bdkconfig.h.win32 	\
	bdkkeysyms-update.pl	\
	bdk.def 		\
	bdkmarshalers.list	\
	bdkmedialib.h		\
	bdkwindowimpl.h		\
	makeenums.pl		\
	makefile.msc		\
	bdk.symbols		\
	makebdkalias.pl		\
	bdkenumtypes.c.template	\
	bdkenumtypes.h.template	\
	abicheck.sh		\
	pltcheck.sh

INCLUDES =                              \
	-DG_LOG_DOMAIN=\"Bdk\"		\
	-DBDK_COMPILATION		\
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	-I$(top_builddir)/bdk		\
	$(BTK_DEBUG_FLAGS) 		\
	$(BDK_DEP_CFLAGS)

gtarget=$(bdktarget)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

# libtool stuff: set version and export symbols for resolving
# since automake doesn't support conditionalized libsomething_la_LDFLAGS
# we use the general approach here
LDADD = \
	$(BTK_LINK_FLAGS)						\
	-version-info $(LT_VERSION_INFO)		 		\
	-export-dynamic 						\
	-rpath $(libdir) 						\
	$(no_undefined)							\
	$(LIBTOOL_EXPORT_OPTIONS)

if USE_MEDIALIB
medialib_sources =  \
    bdkmedialib.c
else
medialib_sources =
endif

#
# setup source file variables
#
#
# BDK header files for public installation (non-generated)
#
bdk_public_h_sources =				\
	bdk.h					\
	bdkapplaunchcontext.h			\
	bdkbairo.h				\
	bdkcolor.h				\
	bdkcursor.h				\
	bdkdisplay.h				\
	bdkdisplaymanager.h			\
	bdkdnd.h				\
	bdkdrawable.h				\
	bdkevents.h				\
	bdkfont.h				\
	bdkgc.h					\
	bdki18n.h				\
	bdkimage.h				\
	bdkinput.h				\
	bdkkeys.h				\
	bdkkeysyms.h				\
	bdkkeysyms-compat.h			\
	bdkbango.h				\
	bdkpixbuf.h				\
	bdkpixmap.h				\
	bdkprivate.h				\
	bdkproperty.h				\
	bdkrebunnyion.h				\
	bdkrgb.h				\
	bdkscreen.h				\
	bdkselection.h				\
	bdkspawn.h				\
	bdktestutils.h				\
	bdktypes.h				\
	bdkvisual.h				\
	bdkwindow.h

bdk_built_public_sources =			\
	bdkenumtypes.h

bdk_built_private_headers =			\
	bdkalias.h

bdk_private_headers =   \
	bdkinternals.h \
	bdkintl.h \
    bdkpoly-generic.h	\
	bdkrebunnyion-generic.h

bdk_c_sources =                 \
	$(medialib_sources)     \
	bdk.c			\
	bdkapplaunchcontext.c	\
	bdkbairo.c		\
	bdkcolor.c		\
	bdkcursor.c		\
	bdkdisplay.c		\
	bdkdisplaymanager.c	\
	bdkdnd.c		\
	bdkdraw.c		\
	bdkevents.c     	\
	bdkfont.c		\
	bdkgc.c			\
	bdkglobals.c		\
	bdkimage.c		\
	bdkkeys.c		\
	bdkkeyuni.c		\
	bdkoffscreenwindow.c	\
	bdkbango.c		\
	bdkpixbuf-drawable.c	\
	bdkpixbuf-render.c	\
	bdkpixmap.c		\
	bdkpolyreg-generic.c	\
	bdkrectangle.c		\
	bdkrebunnyion-generic.c	\
	bdkrgb.c		\
	bdkscreen.c		\
	bdkselection.c		\
	bdkvisual.c		\
	bdkwindow.c		\
	bdkwindowimpl.c

bdk_built_sources =				\
	bdkaliasdef.c				\
	bdkenumtypes.c				\
	bdkmarshalers.h 			\
	bdkmarshalers.c				\
	$(bdk_built_public_sources)		\
	$(bdk_built_private_headers)

#
# setup BDK sources and their dependencies
#

bdkincludedir = $(includedir)/btk-2.0/bdk
bdkinclude_HEADERS = $(bdk_public_h_sources) $(bdk_built_public_sources)

common_sources =                \
	$(bdk_private_headers)      \
    $(bdk_c_sources)	\
	bdkenumtypes.c		\
	bdkmarshalers.c		\
	bdkmarshalers.h

libbdk_directfb_2_0_la_SOURCES = $(common_sources)
libbdk_directfb_2_0_la_LIBADD = directfb/libbdk-directfb.la $(BDK_DEP_LIBS)
libbdk_directfb_2_0_la_LDFLAGS = $(LDADD)

libbdk_x11_2_0_la_SOURCES = $(common_sources)
libbdk_x11_2_0_la_LIBADD = x11/libbdk-x11.la $(BDK_DEP_LIBS)
libbdk_x11_2_0_la_LDFLAGS = $(LDADD)

libbdk_quartz_2_0_la_SOURCES = $(common_sources) bdkkeynames.c
libbdk_quartz_2_0_la_LIBADD = quartz/libbdk-quartz.la $(BDK_DEP_LIBS)
libbdk_quartz_2_0_la_LDFLAGS = $(LDADD)

libbdk_win32_2_0_la_SOURCES = $(common_sources) bdkkeynames.c
libbdk_win32_2_0_la_LIBADD = win32/libbdk-win32.la $(BDK_DEP_LIBS)
libbdk_win32_2_0_la_DEPENDENCIES = win32/libbdk-win32.la win32/rc/bdk-win32-res.o bdk.def
libbdk_win32_2_0_la_LDFLAGS = -Wl,win32/rc/bdk-win32-res.o -export-symbols $(srcdir)/bdk.def $(LDADD)

if HAVE_INTROSPECTION

base_introspection_files = 		\
	$(filter-out bdkkeysyms-compat.h, $(bdk_public_h_sources))	\
	$(bdk_c_sources)	\
	bdkkeynames.c		\
        bdkenumtypes.c		\
        bdkenumtypes.h

introspection_files = $(base_introspection_files)

Bdk-2.0.gir: $(bdktargetlib) Makefile
Bdk_2_0_gir_SCANNERFLAGS = --c-include="bdk/bdk.h"
Bdk_2_0_gir_INCLUDES = Bunnyio-2.0 BdkPixbuf-2.0 Bango-1.0 bairo-1.0
Bdk_2_0_gir_LIBS = $(bdktargetlib)
Bdk_2_0_gir_FILES = $(introspection_files)
Bdk_2_0_gir_EXPORT_PACKAGES = bdk-2.0
Bdk_2_0_gir_CFLAGS = $(INCLUDES)
INTROSPECTION_GIRS += Bdk-2.0.gir

if USE_X11
x11_introspection_files = 		\
	x11/checksettings.c		\
	x11/bdkapplaunchcontext-x11.c	\
	x11/bdkasync.c			\
	x11/bdkcolor-x11.c		\
	x11/bdkcursor-x11.c		\
	x11/bdkdisplay-x11.c		\
	x11/bdkdnd-x11.c		\
	x11/bdkdrawable-x11.c		\
	x11/bdkgc-x11.c			\
	x11/bdkgeometry-x11.c		\
	x11/bdkglobals-x11.c		\
	x11/bdkimage-x11.c		\
	x11/bdkim-x11.c			\
	x11/bdkinput.c			\
	x11/bdkkeys-x11.c		\
	x11/bdkmain-x11.c		\
	x11/bdkpixmap-x11.c		\
	x11/bdkproperty-x11.c		\
	x11/bdkscreen-x11.c		\
	x11/bdkselection-x11.c		\
	x11/bdksettings.c		\
	x11/bdkspawn-x11.c		\
	x11/bdktestutils-x11.c		\
	x11/bdkvisual-x11.c		\
	x11/bdkwindow-x11.c		\
	x11/bdkxftdefaults.c		\
	x11/bdkxid.c			\
	x11/xsettings-client.c		\
	x11/xsettings-common.c		\
        x11/bdkx.h

BdkX11-2.0.gir: $(bdktargetlib) Bdk-2.0.gir Makefile
BdkX11_2_0_gir_SCANNERFLAGS = --identifier-prefix=Bdk --include-uninstalled=Bdk-2.0.gir
BdkX11_2_0_gir_INCLUDES = Bunnyio-2.0 BdkPixbuf-2.0 Bango-1.0 xlib-2.0

BdkX11_2_0_gir_LIBS = $(bdktargetlib)
BdkX11_2_0_gir_FILES = $(x11_introspection_files)
BdkX11_2_0_gir_EXPORT_PACKAGES = bdk-x11-2.0
BdkX11_2_0_gir_CFLAGS = $(INCLUDES) -L$(top_builddir)/bdk
INTROSPECTION_GIRS += BdkX11-2.0.gir

introspection_files += $(filter-out x11/bdkx.h, $(x11_introspection_files))

endif # USE_X11

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/birepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)

endif # HAVE_INTROSPECTION

if OS_WIN32
install-def-file: bdk.def
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/bdk.def $(DESTDIR)$(libdir)/bdk-win32-2.0.def
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/bdk-win32-2.0.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = bdk-win32-$(BTK_API_VERSION).lib

bdk-win32-$(BTK_API_VERSION).lib: libbdk-win32-$(BTK_API_VERSION).la bdk.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libbdk-win32-$(BTK_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:bdk.def -out:$@

install-ms-lib:
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL) bdk-win32-$(BTK_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/bdk-win32-$(BTK_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

# This places the generated .def file in srcdir, since it is expected to be there.
# (The one from a tarball is)
bdk.def: bdk.symbols
	(echo -e EXPORTS; $(CPP) -P -DALL_FILES -DBDK_WINDOWING_WIN32 -DINCLUDE_VARIABLES - <$(srcdir)/bdk.symbols | sed -e '/^$$/d' -e 's/^/	/' -e 's/G_GNUC_[^ ]*//g') > $(srcdir)/bdk.def

bdkalias.h: bdk.symbols
	 $(PERL) $(srcdir)/makebdkalias.pl < $(srcdir)/bdk.symbols > bdkalias.h

bdkaliasdef.c: bdk.symbols
	 $(PERL) $(srcdir)/makebdkalias.pl -def < $(srcdir)/bdk.symbols > bdkaliasdef.c

if OS_LINUX
TESTS = abicheck.sh pltcheck.sh
endif


lib_LTLIBRARIES = $(bdktargetlib)

EXTRA_LTLIBRARIES = libbdk-x11-2.0.la libbdk-win32-2.0.la libbdk-quartz-2.0.la libbdk-directfb-2.0.la

MAINTAINERCLEANFILES = $(bdk_built_sources) stamp-bdkenumtypes.h
EXTRA_DIST += $(bdk_built_sources)
EXTRA_HEADERS =

#
# Rule to install bdkconfig.h header file
#
configexecincludedir = $(libdir)/btk-2.0/include
#configexecinclude_DATA = bdkconfig.h

install-exec-local: bdkconfig.h
	$(mkinstalldirs) $(DESTDIR)$(configexecincludedir)
	file=$(DESTDIR)$(configexecincludedir)/bdkconfig.h; \
	if test -r $$file && cmp -s bdkconfig.h $$file; then :; \
	else $(INSTALL_DATA) bdkconfig.h $$file; fi

install-exec-hook:
if DISABLE_EXPLICIT_DEPS
	$(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/$(bdktargetlib)
endif

#note: not bdkconfig.h
BUILT_SOURCES = \
	$(bdk_built_sources)			\
	bdkconfig.h

bdkenumtypes.h: stamp-bdkenumtypes.h
	@true
stamp-bdkenumtypes.h: @REBUILD@ $(bdk_public_h_sources) bdkenumtypes.h.template
	( cd $(srcdir) && $(BUNNYLIB_MKENUMS) --template bdkenumtypes.h.template \
		$(bdk_public_h_sources) ) >> xgen-geth \
	&& (cmp -s xgen-geth bdkenumtypes.h || cp xgen-geth bdkenumtypes.h ) \
	&& rm -f xgen-geth \
	&& echo timestamp > $(@F)
bdkenumtypes.c: @REBUILD@ $(bdk_public_h_sources) bdkenumtypes.c.template
	( cd $(srcdir) && $(BUNNYLIB_MKENUMS) --template bdkenumtypes.c.template \
		$(bdk_public_h_sources) ) > xgen-getc \
	&& cp xgen-getc bdkenumtypes.c  \
	&& rm -f xgen-getc

# 
# Marshaller generation
#
bdkmarshalers.h: @REBUILD@ bdkmarshalers.list
	$(BUNNYLIB_GENMARSHAL) --prefix=_bdk_marshal $(srcdir)/bdkmarshalers.list --header > bdkmarshalers-h.tmp \
	&& mv bdkmarshalers-h.tmp bdkmarshalers.h \
	|| ( rm -f bdkmarshalers-h.tmp && exit 1)
bdkmarshalers.c: @REBUILD@ bdkmarshalers.list
	(echo "#include \"bdkalias.h\""; $(BUNNYLIB_GENMARSHAL) --prefix=_bdk_marshal $(srcdir)/bdkmarshalers.list --body) > bdkmarshalers-c.tmp	\
	&& mv bdkmarshalers-c.tmp bdkmarshalers.c \
	|| ( rm -f bdkmarshalers-c.tmp && exit 1 )

bdkconfig.h: stamp-gc-h
	@if test -f bdkconfig.h; then :; \
	else rm -f stamp-gc-h; $(MAKE) stamp-gc-h; fi
stamp-gc-h: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status bdk/bdkconfig.h
	echo timestamp > stamp-gc-h

# ---------------- MSVC Projects ----------------
MSVCPROJS = bdk

bdk_FILES = $(libbdk_win32_2_0_la_SOURCES)
bdk_EXCLUDES = bdkdummy

bdk_HEADERS_DIR = $(bdkincludedir)
bdk_HEADERS_INST = $(bdkinclude_HEADERS)
bdk_HEADERS_EXCLUDES = bdkdummy

include $(top_srcdir)/build/Makefile.msvcproj

if HAVE_INTROSPECTION
# Introspection Items for MSVC
MSVC_INTROSPECT_GIRS = Bdk-2.0.gir

BASE_MSVC_GIR_CFLAGS =			\
	-DBDK_COMPILATION		\
	-I../.. -I../../bdk -I.../../bdk/win32

INTROSPECTION_INTERMEDIATE_ITEMS = \
	$(top_builddir)/build/win32/Bdk-2.0.gir.msvc.introspect	\
	$(top_builddir)/build/win32/Bdk_2_0_gir_list

Bdk_2_0_gir_MSVC_FILES = $(filter-out bdkmedialib.c, $(base_introspection_files))
Bdk_2_0_gir_MSVC_EXPORT_PACKAGES = $(Bdk_2_0_gir_EXPORT_PACKAGES)
Bdk_2_0_gir_MSVC_INCLUDE_GIRS = $(Bdk_2_0_gir_INCLUDES)
Bdk_2_0_gir_MSVC_LIBS = bdk-win32-2.0
Bdk_2_0_gir_MSVC_CFLAGS = $(BASE_MSVC_GIR_CFLAGS)
Bdk_2_0_gir_MSVC_SCANNERFLAGS = $(Bdk_2_0_gir_SCANNERFLAGS)

include $(top_srcdir)/build/Makefile.msvc-introspection

else
INTROSPECTION_INTERMEDIATE_ITEMS =
endif

dist-hook:	\
	$(top_builddir)/build/win32/vs9/bdk.vcproj	\
	$(top_builddir)/build/win32/vs9/bdk.headers	\
	$(INTROSPECTION_INTERMEDIATE_ITEMS)

DISTCLEANFILES = bdkconfig.h stamp-gc-h

install-data-local: install-ms-lib install-def-file

uninstall-local: uninstall-ms-lib uninstall-def-file
	rm -f $(DESTDIR)$(configexecincludedir)/bdkconfig.h

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done

-include $(top_srcdir)/git.mk
