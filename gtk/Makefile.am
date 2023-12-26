# Makefile.am for btk+/btk
include $(top_srcdir)/Makefile.decl
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = \
	--warn-all \
	--add-include-path=../bdk
INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(srcdir) \
    --includedir=. \
    --includedir=../bdk

if USE_QUARTZ
BTK_PRINT_PREVIEW_COMMAND="open -b com.apple.Preview %f"
else
BTK_PRINT_PREVIEW_COMMAND="evince --unlink-tempfile --preview --print-settings %s %f"
endif

SUBDIRS = theme-bits . tests
DIST_SUBDIRS=theme-bits tests

if HAVE_PAPI_CUPS
BTK_PRINT_BACKENDS=file,papi,cups
else
if HAVE_CUPS
BTK_PRINT_BACKENDS=file,cups
else
if HAVE_PAPI
BTK_PRINT_BACKENDS=file,papi
else
BTK_PRINT_BACKENDS=file,lpr
endif
endif
endif

BTK_BASE_CFLAGS_DEFINES =				\
	-DBTK_VERSION=\"$(BTK_VERSION)\"		\
	-DBTK_BINARY_VERSION=\"$(BTK_BINARY_VERSION)\"	\
	-DBTK_COMPILATION				\
	-DBTK_DISABLE_DEPRECATED			\
	-DBTK_FILE_SYSTEM_ENABLE_UNSUPPORTED		\
	-DBTK_PRINT_BACKEND_ENABLE_UNSUPPORTED

BTK_PLAT_CFLAGS_DEFINES =				\
	-DBTK_LIBDIR=\"$(libdir)\"			\
	-DBTK_DATADIR=\"$(datadir)\"			\
	-DBTK_DATA_PREFIX=\"$(prefix)\"			\
	-DBTK_SYSCONFDIR=\"$(sysconfdir)\"		\
	-DBTK_HOST=\"$(host)\"				\
	-DBTK_PRINT_BACKENDS=\"$(BTK_PRINT_BACKENDS)\"	\
	-DBTK_PRINT_PREVIEW_COMMAND=\"$(BTK_PRINT_PREVIEW_COMMAND)\"

INCLUDES =						\
	$(BTK_BASE_CFLAGS_DEFINES)			\
	$(BTK_PLAT_CFLAGS_DEFINES)			\
	-DG_LOG_DOMAIN=\"Btk\"				\
	-I$(top_builddir)				\
	-I$(top_builddir)/btk				\
	-I$(top_srcdir) -I../bdk			\
	-I$(top_srcdir)/bdk				\
	$(BMODULE_CFLAGS)				\
	$(BTK_DEBUG_FLAGS)				\
	$(BTK_DEP_CFLAGS)				\
	$(INCLUDED_IMMODULE_DEFINE)

gtarget=$(bdktarget)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
btk_def = btk.def
btk_win32_symbols = -export-symbols btk.def

btk_win32_res = btk-win32-res.o
btk_win32_res_ldflag = -Wl,btk-win32-res.o

btk-win32-res.o : btk-win32.rc
	$(WINDRES) btk-win32.rc $@

install-def-file: btk.def
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL) btk.def $(DESTDIR)$(libdir)/btk-win32-2.0.def
uninstall-def-file:
	-rm $(DESTDIR)$(libdir)/btk-win32-2.0.def
else
install-def-file:
uninstall-def-file:
endif

if MS_LIB_AVAILABLE
noinst_DATA = btk-win32-$(BTK_API_VERSION).lib

btk-win32-$(BTK_API_VERSION).lib: libbtk-win32-$(BTK_API_VERSION).la btk.def
	lib -machine:@LIB_EXE_MACHINE_FLAG@ -name:libbtk-win32-$(BTK_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:btk.def -out:$@

install-ms-lib:
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL) btk-win32-$(BTK_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/btk-win32-$(BTK_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

# btk.def has to be put in builddir and should not be included in a source
# tarball because it differs in 32-bit and 64-bit Windows builds.
btk.def: btk.symbols
	(echo -e EXPORTS; $(CPP) -P -DINCLUDE_VARIABLES -DG_OS_WIN32 -DALL_FILES - <$(srcdir)/btk.symbols | sed -e '/^$$/d' -e 's/^/	/' -e 's/G_GNUC_[^ ]*//g') > btk.def

btkalias.h: btk.symbols
	  $(PERL) $(srcdir)/makebtkalias.pl < $(srcdir)/btk.symbols > btkalias.h
btkaliasdef.c: btk.symbols
	  $(PERL) $(srcdir)/makebtkalias.pl -def < $(srcdir)/btk.symbols > btkaliasdef.c

check-aliases:
	@$(TESTS_ENVIRONMENT) $(srcdir)/aliasfilescheck.sh

.PHONY: check-aliases

TESTS_ENVIRONMENT = srcdir="$(srcdir)" btk_all_c_sources="$(btk_all_c_sources)"
TESTS = aliasfilescheck.sh
if OS_LINUX
TESTS += abicheck.sh pltcheck.sh
endif

libbtkincludedir = $(includedir)/btk-2.0/btk
libadd =								\
	$(top_builddir)/bdk/$(bdktargetlib)				\
	$(BMODULE_LIBS) \
	$(BTK_DEP_LIBS)
deps =

# libtool stuff: set version and export symbols for resolving
# since automake doesn't support conditionalized libsomething_la_LDFLAGS
# we use the general approach here
libtool_opts =							\
  -version-info $(LT_VERSION_INFO)				\
  -export-dynamic $(no_undefined) $(LIBTOOL_EXPORT_OPTIONS)	\
  -rpath $(libdir) $(libbtk_target_ldflags)


included-modules:
if HAVE_INCLUDED_IMMMODULES
	@cd $(top_builddir)/modules/input && $(MAKE) $(AM_MAKEFLAGS) included-modules

libadd += $(INCLUDED_IMMODULE_OBJ)
deps += $(INCLUDED_IMMODULE_OBJ)

$(INCLUDED_IMMODULE_OBJ): included-modules
	@true
endif

.PHONY: included-modules

#
# setup source file variables
#

# BTK+ header files for public installation (non-generated, or generated
# by configure)
btk_public_h_sources =          \
	btk.h			\
	btkaboutdialog.h	\
	btkaccelgroup.h		\
	btkaccellabel.h		\
	btkaccelmap.h		\
	btkaccessible.h		\
	btkaction.h		\
	btkactiongroup.h	\
	btkactivatable.h	\
	btkadjustment.h		\
	btkalignment.h		\
	btkarrow.h		\
	btkaspectframe.h	\
	btkassistant.h		\
	btkbbox.h		\
	btkbin.h		\
	btkbindings.h		\
	btkbox.h		\
	btkbuilder.h		\
	btkbuildable.h		\
	btkbutton.h		\
	btkcalendar.h		\
	btkcelleditable.h	\
	btkcelllayout.h		\
	btkcellrenderer.h	\
	btkcellrendereraccel.h	\
	btkcellrenderercombo.h	\
	btkcellrendererpixbuf.h	\
	btkcellrendererprogress.h \
	btkcellrendererspin.h   \
	btkcellrendererspinner.h\
	btkcellrenderertext.h	\
	btkcellrenderertoggle.h	\
	btkcellview.h		\
	btkcheckbutton.h	\
	btkcheckmenuitem.h	\
	btkclipboard.h		\
	btkcolorbutton.h	\
	btkcolorsel.h		\
	btkcolorseldialog.h	\
	btkcombobox.h		\
	btkcomboboxentry.h	\
	btkcomboboxtext.h	\
	btkcontainer.h		\
	btkdebug.h              \
	btkdialog.h		\
	btkdnd.h		\
	btkdrawingarea.h	\
	btkeditable.h           \
	btkentry.h		\
	btkentrybuffer.h	\
	btkentrycompletion.h	\
	btkenums.h		\
	btkeventbox.h		\
	btkexpander.h		\
	btkfilechooser.h        \
	btkfilechooserbutton.h  \
	btkfilechooserdialog.h  \
	btkfilechooserwidget.h  \
	btkfilefilter.h		\
	btkfixed.h		\
	btkfontbutton.h		\
	btkfontsel.h		\
	btkframe.h		\
	btkgc.h			\
	btkhandlebox.h		\
	btkhbbox.h		\
	btkhbox.h		\
	btkhpaned.h		\
	btkhruler.h		\
	btkhscale.h		\
	btkhscrollbar.h		\
	btkhseparator.h		\
	btkhsv.h		\
	btkiconfactory.h	\
	btkicontheme.h		\
	btkiconview.h		\
	btkimage.h		\
	btkimagemenuitem.h	\
	btkimcontext.h		\
	btkimcontextsimple.h	\
	btkimmodule.h		\
	btkimmulticontext.h	\
	btkinfobar.h		\
	btkinvisible.h		\
	btkitem.h		\
	btklabel.h		\
	btklayout.h             \
	btklinkbutton.h		\
	btkliststore.h		\
	btkmain.h		\
	btkmenu.h		\
	btkmenubar.h		\
	btkmenuitem.h		\
	btkmenushell.h		\
	btkmenutoolbutton.h	\
	btkmessagedialog.h	\
	btkmisc.h		\
	btkmodules.h		\
	btkmountoperation.h     \
	btknotebook.h		\
	btkobject.h		\
	btkoffscreenwindow.h	\
	btkorientable.h		\
	btkpagesetup.h		\
	btkpaned.h		\
	btkpapersize.h		\
	btkplug.h		\
	btkprintcontext.h	\
	btkprintoperation.h	\
	btkprintoperationpreview.h	\
	btkprintsettings.h	\
	btkprivate.h		\
	btkprogressbar.h	\
	btkradioaction.h	\
	btkradiobutton.h	\
	btkradiomenuitem.h	\
	btkradiotoolbutton.h	\
	btkrange.h		\
	btkrc.h			\
	btkrecentaction.h	\
	btkrecentchooser.h 	\
	btkrecentchooserdialog.h \
	btkrecentchoosermenu.h 	\
	btkrecentchooserwidget.h \
	btkrecentfilter.h 	\
	btkrecentmanager.h 	\
	btkruler.h		\
	btkscale.h		\
	btkscalebutton.h	\
	btkscrollbar.h		\
	btkscrolledwindow.h	\
	btkselection.h		\
	btkseparator.h		\
	btkseparatormenuitem.h	\
	btkseparatortoolitem.h	\
	btkshow.h		\
	btksettings.h		\
	btksizegroup.h		\
	btksocket.h		\
	btkspinbutton.h		\
	btkspinner.h		\
	btkstatusbar.h		\
	btkstatusicon.h		\
	btkstock.h		\
	btkstyle.h		\
	btktable.h		\
	btktearoffmenuitem.h    \
	btktestutils.h		\
	btktextbuffer.h		\
	btktextbufferrichtext.h	\
	btktextchild.h		\
	btktextdisplay.h	\
	btktextiter.h		\
	btktextmark.h		\
	btktexttag.h		\
	btktexttagtable.h	\
	btktextview.h		\
	btktoggleaction.h	\
	btktogglebutton.h	\
	btktoggletoolbutton.h	\
	btktoolbar.h		\
	btktoolbutton.h		\
	btktoolitem.h		\
	btktoolitemgroup.h	\
	btktoolpalette.h	\
	btktoolshell.h		\
	btktooltip.h		\
	btktreednd.h		\
	btktreemodel.h		\
	btktreemodelfilter.h	\
	btktreemodelsort.h	\
	btktreeselection.h	\
	btktreesortable.h	\
	btktreestore.h		\
	btktreeview.h		\
	btktreeviewcolumn.h	\
	btktypeutils.h		\
	btkuimanager.h		\
	btkvbbox.h		\
	btkvbox.h		\
	btkviewport.h		\
	btkvolumebutton.h	\
	btkvpaned.h		\
	btkvruler.h		\
	btkvscale.h		\
	btkvscrollbar.h		\
	btkvseparator.h		\
	btkwidget.h		\
	btkwindow.h

if OS_UNIX
btk_unix_print_public_h_sources =    \
	btkpagesetupunixdialog.h	\
	btkprintunixdialog.h		\
	btkprinter.h			\
	btkprintjob.h			\
	btkunixprint.h
endif

# Installed header files without compatibility guarantees
# that are not included in btk/btk.h
btk_semi_private_h_sources =    \
	btktextlayout.h

# BTK+ header files that don't get installed
btk_private_h_sources =		\
	btkquery.h		\
	btksearchengine.h	\
	btksearchenginesimple.h	\
	btkdndcursors.h		\
	btkentryprivate.h	\
	btkbuilderprivate.h 	\
	btkcustompaperunixdialog.h\
	btkfilechooserdefault.h	\
	btkfilechooserembed.h	\
	btkfilechooserentry.h	\
	btkfilechooserprivate.h	\
	btkfilechoosersettings.h \
	btkfilechooserutils.h	\
	btkfilesystem.h		\
	btkfilesystemmodel.h	\
	btkiconcache.h		\
	btkimcontextsimpleseqs.h   \
	btkintl.h		\
	btkkeyhash.h		\
	btkmnemonichash.h	\
	btkmountoperationprivate.h \
	btkbango.h		\
	btkpathbar.h		\
	btkplugprivate.h	\
	btkprintoperation-private.h\
	btkprintutils.h		\
	btkrbtree.h		\
	btkrecentchooserdefault.h \
	btkrecentchooserprivate.h \
	btkrecentchooserutils.h \
	btksocketprivate.h	\
	btktextbtree.h		\
	btktextbufferserialize.h\
	btktextchildprivate.h	\
	btktextiterprivate.h	\
	btktextmarkprivate.h	\
	btktextsegment.h	\
	btktexttagprivate.h	\
	btktexttypes.h		\
	btktextutil.h		\
	btkthemes.h		\
	btktoggleactionprivate.h\
	btktoolpaletteprivate.h	\
	btktreedatalist.h	\
	btktreeprivate.h	\
	btkwindow-decorate.h	\
	$(btk_clipboard_dnd_h_sources)

# BTK+ C sources to build the library from
btk_base_c_sources =            \
	btkquery.c		\
	btksearchengine.c	\
	btksearchenginesimple.c	\
	fnmatch.c		\
	btkaboutdialog.c	\
	btkaccelgroup.c		\
	btkaccellabel.c		\
	btkaccelmap.c		\
	btkaccessible.c		\
	btkaction.c		\
	btkactiongroup.c	\
	btkactivatable.c	\
	btkadjustment.c		\
	btkalignment.c		\
	btkarrow.c		\
	btkaspectframe.c	\
	btkassistant.c		\
	btkbbox.c		\
	btkbin.c		\
	btkbindings.c		\
	btkbox.c		\
	btkbuildable.c		\
	btkbuilder.c		\
	btkbuilderparser.c	\
	btkbutton.c		\
	btkcalendar.c		\
	btkcelleditable.c	\
	btkcelllayout.c		\
	btkcellrenderer.c	\
	btkcellrendereraccel.c	\
	btkcellrenderercombo.c	\
	btkcellrendererpixbuf.c	\
	btkcellrendererprogress.c \
	btkcellrendererspin.c   \
	btkcellrendererspinner.c\
	btkcellrenderertext.c	\
	btkcellrenderertoggle.c	\
	btkcellview.c		\
	btkcheckbutton.c	\
	btkcheckmenuitem.c	\
	btkcolorbutton.c	\
	btkcolorsel.c		\
	btkcolorseldialog.c	\
	btkcombobox.c		\
	btkcomboboxentry.c	\
	btkcomboboxtext.c 	\
	btkcontainer.c		\
	btkdialog.c		\
	btkdrawingarea.c	\
	btkeditable.c           \
	btkentry.c		\
	btkentrybuffer.c	\
	btkentrycompletion.c	\
	btkeventbox.c		\
	btkexpander.c		\
	btkfilechooser.c	\
	btkfilechooserbutton.c	\
	btkfilechooserdefault.c	\
	btkfilechooserdialog.c	\
	btkfilechooserembed.c	\
	btkfilechooserentry.c	\
	btkfilechoosersettings.c \
	btkfilechooserutils.c	\
	btkfilechooserwidget.c	\
	btkfilefilter.c		\
	btkfilesystem.c		\
	btkfilesystemmodel.c	\
	btkfixed.c		\
	btkfontbutton.c         \
	btkfontsel.c            \
	btkframe.c		\
	btkgc.c			\
	btkhandlebox.c		\
	btkhbbox.c		\
	btkhbox.c		\
	btkhpaned.c		\
	btkhruler.c		\
	btkhscale.c		\
	btkhscrollbar.c		\
	btkhseparator.c		\
	btkhsv.c		\
	btkiconcache.c		\
	btkiconcachevalidator.c	\
	btkiconfactory.c	\
	btkicontheme.c		\
	btkiconview.c		\
	btkimage.c		\
	btkimagemenuitem.c	\
	btkimcontext.c		\
	btkimcontextsimple.c	\
	btkimmodule.c		\
	btkimmulticontext.c	\
	btkinfobar.c		\
	btkinputdialog.c	\
	btkinvisible.c		\
	btkitem.c		\
	btkkeyhash.c		\
	btklabel.c		\
	btklayout.c		\
	btklinkbutton.c		\
	btkliststore.c		\
	btkmain.c		\
	btkmarshal.c		\
	btkmarshalers.c		\
	btkmenu.c		\
	btkmenubar.c		\
	btkmenuitem.c		\
	btkmenushell.c		\
	btkmenutoolbutton.c	\
	btkmessagedialog.c	\
	btkmisc.c		\
	btkmnemonichash.c	\
	btkmodules.c		\
	btkmountoperation.c     \
	btknotebook.c		\
	btkobject.c		\
	btkoffscreenwindow.c	\
	btkorientable.c		\
	btkpagesetup.c		\
	btkpaned.c		\
	btkbango.c		\
	btkpapersize.c		\
	btkpathbar.c		\
	btkplug.c		\
	btkprintcontext.c	\
	btkprintoperation.c	\
	btkprintoperationpreview.c	\
	btkprintsettings.c	\
	btkprintutils.c		\
	btkprogressbar.c	\
	btkradioaction.c	\
	btkradiobutton.c	\
	btkradiomenuitem.c	\
	btkradiotoolbutton.c	\
	btkrange.c		\
	btkrbtree.c 		\
	btkrc.c			\
	btkrecentaction.c	\
	btkrecentchooserdefault.c \
	btkrecentchooserdialog.c \
	btkrecentchoosermenu.c 	\
	btkrecentchooserwidget.c \
	btkrecentchooserutils.c \
	btkrecentchooser.c 	\
	btkrecentfilter.c 	\
	btkrecentmanager.c 	\
	btkruler.c		\
	btkscale.c		\
	btkscalebutton.c	\
	btkscrollbar.c		\
	btkscrolledwindow.c	\
	btkselection.c		\
	btkseparator.c		\
	btkseparatormenuitem.c	\
	btkseparatortoolitem.c	\
	btksettings.c		\
	btksizegroup.c		\
	btkshow.c		\
	btksocket.c		\
	btkspinbutton.c		\
	btkspinner.c		\
	btkstatusbar.c		\
	btkstatusicon.c		\
	btkstock.c		\
	btkstyle.c		\
	btktable.c		\
	btktearoffmenuitem.c    \
	btktestutils.c		\
	btktextbtree.c		\
	btktextbuffer.c		\
	btktextbufferrichtext.c	\
	btktextbufferserialize.c\
	btktextchild.c		\
	btktextdisplay.c	\
	btktextiter.c		\
	btktextlayout.c		\
	btktextmark.c		\
	btktextsegment.c	\
	btktexttag.c		\
	btktexttagtable.c	\
	btktexttypes.c		\
	btktextutil.c		\
	btktextview.c		\
	btkthemes.c		\
	btktoggleaction.c	\
	btktogglebutton.c	\
	btktoggletoolbutton.c	\
	btktoolbar.c		\
	btktoolbutton.c		\
	btktoolitem.c		\
	btktoolitemgroup.c		\
	btktoolpalette.c	\
	btktoolshell.c		\
	btktooltip.c		\
	btktreedatalist.c	\
	btktreednd.c		\
	btktreemodel.c		\
	btktreemodelfilter.c	\
	btktreemodelsort.c	\
	btktreeselection.c	\
	btktreesortable.c	\
	btktreestore.c		\
	btktreeview.c		\
	btktreeviewcolumn.c	\
	btktypebuiltins.c	\
	btktypeutils.c		\
	btkuimanager.c		\
	btkvbbox.c		\
	btkvbox.c		\
	btkvolumebutton.c	\
	btkviewport.c		\
	btkvpaned.c		\
	btkvruler.c		\
	btkvscale.c		\
	btkvscrollbar.c		\
	btkvseparator.c		\
	btkwidget.c		\
	btkwindow-decorate.c    \
	btkwindow.c		\
	$(btk_clipboard_dnd_c_sources)

# Broken
btk_public_h_sources += \
	btktext.h		\
	btktree.h		\
	btktreeitem.h
btk_base_c_sources += \
	btktext.c		\
	btktree.c		\
	btktreeitem.c

# Deprecated
btk_public_h_sources += \
	btkclist.h		\
	btkcombo.h		\
	btkctree.h		\
	btkcurve.h		\
	btkfilesel.h		\
	btkgamma.h		\
	btkinputdialog.h	\
	btkitemfactory.h	\
	btklist.h		\
	btklistitem.h		\
	btkoldeditable.h	\
	btkoptionmenu.h		\
	btkpixmap.h		\
	btkpreview.h		\
	btkprogress.h		\
	btksignal.h		\
	btktipsquery.h		\
	btktooltips.h
btk_base_c_sources += \
	btkclist.c		\
	btkcombo.c		\
	btkctree.c		\
	btkcurve.c		\
	btkfilesel.c		\
	btkgamma.c		\
	btkitemfactory.c	\
	btklist.c		\
	btklistitem.c		\
	btkoldeditable.c	\
	btkoptionmenu.c		\
	btkpixmap.c		\
	btkpreview.c		\
	btkprogress.c		\
	btksignal.c		\
	btktipsquery.c		\
	btktooltips.c

btk_c_sources = $(btk_base_c_sources)
btk_all_c_sources = $(btk_base_c_sources)

btk_os_unix_c_sources = \
	btkcustompaperunixdialog.c	\
	btkpagesetupunixdialog.c	\
	btkprinter.c			\
	btkprinteroption.c		\
	btkprinteroptionset.c		\
	btkprinteroptionwidget.c	\
	btkprintjob.c			\
	btkprintoperation-unix.c	\
	btkprintunixdialog.c		\
        btkprintbackend.c		\
	btksearchenginebeagle.c		\
	btksearchenginetracker.c
btk_all_c_sources += $(btk_os_unix_c_sources)
if OS_UNIX
btk_private_h_sources += \
	btkiconcachevalidator.h		\
	btkprintbackend.h		\
	btkprinter-private.h		\
	btkprinteroption.h		\
	btkprinteroptionset.h		\
	btkprinteroptionwidget.h	\
	btksearchenginebeagle.h		\
	btksearchenginetracker.h
btk_c_sources += $(btk_os_unix_c_sources)
endif

btk_os_win32_c_sources = \
	btkprint-win32.c		\
	btkprintoperation-win32.c
btk_all_c_sources += $(btk_os_win32_c_sources)
if OS_WIN32
btk_private_h_sources += btkprint-win32.h
btk_c_sources += $(btk_os_win32_c_sources)
endif

btk_use_x11_c_sources = \
	btkplug-x11.c   \
	btksocket-x11.c \
	btkxembed.c     \
	btktrayicon-x11.c \
	btkmountoperation-x11.c
btk_use_win32_c_sources = \
	btkplug-win32.c   \
	btksocket-win32.c \
	btkwin32embed.c   \
	btkwin32embedwidget.c \
	btkmountoperation-stub.c
btk_use_quartz_c_sources =	\
	btksearchenginequartz.c	\
	btkplug-stub.c		\
	btksocket-stub.c	\
	btkmountoperation-stub.c
btk_use_stub_c_sources = \
	btkplug-stub.c   \
	btksocket-stub.c \
	btkmountoperation-stub.c
btk_all_c_sources += $(btk_use_x11_c_sources) $(btk_use_win32_c_sources) $(btk_use_stub_c_sources)
if USE_X11
btk_private_h_sources += btkxembed.h btktrayicon.h xembed.h
btk_c_sources += $(btk_use_x11_c_sources)
else
if USE_WIN32
btk_private_h_sources += btkwin32embed.h btkwin32embedwidget.h
btk_c_sources += $(btk_use_win32_c_sources)
else
if USE_QUARTZ
libbtk_quartz_2_0_la_CFLAGS = "-xobjective-c"
btk_private_h_sources += btksearchenginequartz.h
btk_c_sources += $(btk_use_quartz_c_sources)
else
btk_c_sources += $(btk_use_stub_c_sources)
endif
endif
endif

if USE_QUARTZ
btk_clipboard_dnd_c_sources = btkclipboard-quartz.c btkdnd-quartz.c btkquartz.c
btk_clipboard_dnd_h_sources = btkquartz.h
else
btk_clipboard_dnd_c_sources = btkclipboard.c btkdnd.c
endif
EXTRA_DIST += btkquartz.h

# we use our own built_sources variable rules to avoid automake's
# BUILT_SOURCES oddities
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content

# built sources that get installed with the header files
btk_built_public_sources =			\
	btkmarshal.h				\
	btktypebuiltins.h

# built headers that don't get installed
btk_built_private_headers =			\
	btkalias.h				\
	btkmarshalers.h				\
	btkbuiltincache.h

btk_built_sources =				\
	btkaliasdef.c				\
	btktypebuiltins.c			\
	btktypefuncs.c				\
	btkmarshalers.c				\
	btkmarshal.c				\
	${btk_built_private_headers}		\
	${btk_built_public_sources}

stamp_files =					\
	stamp-btkmarshalers.h			\
	stamp-btkmarshal.h			\
	stamp-btktypebuiltins.h			\
	stamp-icons

# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
btk_extra_sources =				\
	paper_names.c				\
	paper_names_offsets.c			\
	gen-paper-names.c			\
	btkstatusicon-quartz.c                  \
	btk.symbols				\
	btkversion.h.in				\
	btkmarshalers.list			\
	btkmarshal.list

#
# setup BTK+ sources and their dependencies
#
MAINTAINERCLEANFILES = $(btk_built_sources) $(stamp_files) $(GENERATED_ICONS) stock-icons/icon-theme.cache
EXTRA_HEADERS =
EXTRA_DIST += $(btk_private_h_sources) $(btk_extra_sources)
EXTRA_DIST += $(btk_built_sources)
EXTRA_DIST += $(STOCK_ICONS)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-gdef xgen-gtbh xgen-gtic xgen-gmh xgen-gmc xgen-gmlh xgen-gmlc xgen-gtfsrc.c xgen-gtf
CLEANFILES = $(gen_sources) $(btk_def)

BUILT_SOURCES = $(btk_built_sources)

# all autogenerated files need to be generated in the srcdir,
# so old versions get remade and are not confused with newer
# versions in the build dir. thus a development setup requires
# srcdir to be writable, passing --disable-rebuilds to
# ../configure will supress all autogeneration rules.
btkmarshalers.h: stamp-btkmarshalers.h
	@true
stamp-btkmarshalers.h: @REBUILD@ btkmarshalers.list
	$(BUNNYLIB_GENMARSHAL) --prefix=_btk_marshal $(srcdir)/btkmarshalers.list --header >> xgen-gmlh \
	&& (cmp -s xgen-gmlh btkmarshalers.h || cp xgen-gmlh btkmarshalers.h) \
	&& rm -f xgen-gmlh \
	&& echo timestamp > $(@F)	
btkmarshalers.c: @REBUILD@ btkmarshalers.list
	(echo "#include \"btkmarshalers.h\""; \
	 echo "#include \"btkalias.h\""; \
	 $(BUNNYLIB_GENMARSHAL) --prefix=_btk_marshal $(srcdir)/btkmarshalers.list --body) >> xgen-gmlc \
	&& cp xgen-gmlc btkmarshalers.c \
	&& rm -f xgen-gmlc

btkmarshal.h: stamp-btkmarshal.h
	@true
stamp-btkmarshal.h: @REBUILD@ btkmarshal.list
	echo "#ifndef BTK_DISABLE_DEPRECATED" > xgen-gmh					\
        && $(BUNNYLIB_GENMARSHAL) --prefix=btk_marshal $(srcdir)/btkmarshal.list --header >> xgen-gmh \
	&& echo "#endif /* BTK_DISABLE_DEPRECATED */" >> xgen-gmh \
	&& (cmp -s xgen-gmh btkmarshal.h || cp xgen-gmh btkmarshal.h) \
	&& rm -f xgen-gmh \
	&& echo timestamp > $(@F)
btkmarshal.c: @REBUILD@ btkmarshal.list
	(echo "#include \"btkmarshal.h\""; \
	 echo "#include \"btkalias.h\""; \
	 $(BUNNYLIB_GENMARSHAL) --prefix=btk_marshal $(srcdir)/btkmarshal.list --body; \
	 echo "#define __btk_marshal_MARSHAL_C__"; \
	 echo "#include \"btkaliasdef.c\"") >> xgen-gmc \
	&& cp xgen-gmc btkmarshal.c \
	&& rm -f xgen-gmc

btktypebuiltins.h: stamp-btktypebuiltins.h
	@true
stamp-btktypebuiltins.h: @REBUILD@ $(btk_public_h_sources) btktypebuiltins.h.template
	( cd $(srcdir) && $(BUNNYLIB_MKENUMS) --template btktypebuiltins.h.template \
		$(btk_public_h_sources) ) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh btktypebuiltins.h || cp xgen-gtbh btktypebuiltins.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)
btktypebuiltins.c: @REBUILD@ $(btk_public_h_sources) btktypebuiltins.c.template
	( cd $(srcdir) && $(BUNNYLIB_MKENUMS) --template btktypebuiltins.c.template \
		$(btk_public_h_sources) ) > xgen-gtbc \
	&& cp xgen-gtbc btktypebuiltins.c  \
	&& rm -f xgen-gtbc

btktypefuncs.c: @REBUILD@ $(top_srcdir)/btk/*.h $(top_srcdir)/bdk/*.h Makefile
	echo '#include <btk/btk.h>' > xgen-gtfsrc.c && \
	  ${CPP} $(DEFS) $(INCLUDES) -DBTK_ENABLE_BROKEN $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) xgen-gtfsrc.c | \
	  grep '^GType\s*g[td]k_[a-zA-Z0-9_]*_get_type\s*(void)' | \
	  grep -o '\bg[td]k_[a-zA-Z0-9_]*_get_type\b' | \
	  sed '{ s/^/*tp++ = /; s/$$/();/; }' | \
	  sort > xgen-gtf \
	&& cp xgen-gtf $@ && rm -f xgen-gtf
btktestutils.c: btktypefuncs.c

# target platform:
lib_LTLIBRARIES = $(btktargetlib)

btkincludedir = $(includedir)/btk-2.0/btk
btkinclude_HEADERS = $(btk_public_h_sources) $(btk_semi_private_h_sources) $(btk_built_public_sources) btkversion.h

btkunixprintincludedir = $(includedir)/btk-unix-print-2.0/btk
btkunixprintinclude_HEADERS = $(btk_unix_print_public_h_sources)

libbtk_x11_2_0_la_SOURCES = $(btk_c_sources)
libbtk_win32_2_0_la_SOURCES = $(btk_c_sources)
libbtk_quartz_2_0_la_SOURCES = $(btk_c_sources)
libbtk_directfb_2_0_la_SOURCES = $(btk_c_sources)

libbtk_x11_2_0_la_LDFLAGS = $(libtool_opts)
libbtk_win32_2_0_la_LDFLAGS = $(libtool_opts) -Wl,-luuid
libbtk_quartz_2_0_la_LDFLAGS = $(libtool_opts)
libbtk_directfb_2_0_la_LDFLAGS = $(libtool_opts)

libbtk_x11_2_0_la_LIBADD = $(libadd)
libbtk_win32_2_0_la_LIBADD = $(libadd) -lole32 -lgdi32 -lcomdlg32 -lwinspool -lcomctl32
libbtk_quartz_2_0_la_LIBADD = $(libadd)
libbtk_directfb_2_0_la_LIBADD = $(libadd)

libbtk_x11_2_0_la_DEPENDENCIES = $(deps)
libbtk_win32_2_0_la_DEPENDENCIES = $(btk_def) $(btk_win32_res) $(deps)
libbtk_quartz_2_0_la_DEPENDENCIES = $(deps)
libbtk_directfb_2_0_la_DEPENDENCIES = $(deps)

if USE_WIN32
libbtk_target_ldflags = $(btk_win32_res_ldflag) $(btk_win32_symbols)
endif
EXTRA_LTLIBRARIES = libbtk-x11-2.0.la libbtk-win32-2.0.la libbtk-quartz-2.0.la libbtk-directfb-2.0.la

install-exec-hook: 
if DISABLE_EXPLICIT_DEPS
	$(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/$(btktargetlib)
endif

if USE_QUARTZ
install-mac-key-theme:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Mac/btk-2.0-key
	$(INSTALL_DATA) $(srcdir)/btkrc.key.mac $(DESTDIR)$(datadir)/themes/Mac/btk-2.0-key/btkrc
uninstall-mac-key-theme:
	rm -f $(DESTDIR)$(datadir)/themes/Mac/btk-2.0-key/btkrc
else
install-mac-key-theme:
uninstall-mac-key-theme:
endif

# -------------- MSVC Projects ----------------
MSVCPROJS = btk

btk_FILES = $(btk_base_c_sources) $(btk_os_win32_c_sources) $(btk_use_win32_c_sources)
btk_EXCLUDES = btkdummy

btk_HEADERS_DIR = $(libbtkincludedir)
btk_HEADERS_INST = $(btkinclude_HEADERS)
btk_HEADERS_EXCLUDES = btkdummy

include $(top_srcdir)/build/Makefile.msvcproj

if HAVE_INTROSPECTION
# Introspection Items for MSVC
MSVC_INTROSPECT_GIRS = Btk-2.0.gir

BTK_MSVC_GIR_CFLAGS =				\
	$(BTK_BASE_CFLAGS_DEFINES)				\
	-DBTK_LIBDIR=\\\"/dummy/lib\\\"				\
	-DBTK_DATADIR=\\\"/dummy/share\\\"			\
	-DBTK_DATA_PREFIX=\\\"/dummy\\\"			\
	-DBTK_SYSCONFDIR=\\\"/dummy/etc\\\"			\
	-DBTK_HOST=\\\"'$$$$(AT_PLAT)'-pc-vs'$$$$(VSVER)'\\\"	\
	-DBTK_PRINT_BACKENDS=\\\"file\\\"			\
	-DINCLUDE_IM_am_et			\
	-DINCLUDE_IM_cedilla			\
	-DINCLUDE_IM_cyrillic_translit		\
	-DINCLUDE_IM_ime			\
	-DINCLUDE_IM_inuktitu			\
	-DINCLUDE_IM_ipa			\
	-DINCLUDE_IM_multipress			\
	-DINCLUDE_IM_thai			\
	-DINCLUDE_IM_ti_er			\
	-DINCLUDE_IM_ti_et			\
	-DINCLUDE_IM_viqr			\
	-UBDK_DISABLE_DEPRECATED		\
	-UBTK_DISABLE_DEPRECATED		\
	-DBTK_TEXT_USE_INTERNAL_UNSUPPORTED_API	\
	-I../.. -I../../btk -I../../bdk

INTROSPECTION_INTERMEDIATE_ITEMS = \
	$(top_builddir)/build/win32/Btk-2.0.gir.msvc.introspect		\
	$(top_builddir)/build/win32/Btk_2_0_gir_list

Btk_2_0_gir_MSVC_FILES =		\
	$(btkinclude_HEADERS)		\
	$(btk_base_c_sources)		\
	$(btk_os_win32_c_sources)	\
	$(btk_use_win32_c_sources)	\
	btktypebuiltins.h		\
	btktypebuiltins.c

Btk_2_0_gir_MSVC_EXPORT_PACKAGES = $(Btk_2_0_gir_EXPORT_PACKAGES)
Btk_2_0_gir_MSVC_INCLUDE_GIRS = Batk-1.0
Btk_2_0_gir_MSVC_LIBS = btk-win32-2.0 bdk-win32-2.0
Btk_2_0_gir_MSVC_CFLAGS = $(BTK_MSVC_GIR_CFLAGS)
Btk_2_0_gir_MSVC_SCANNERFLAGS =	\
	--warn-all	\
	--include-uninstalled='./vs$$$$(VSVER)/$$$$(CFG)/$$$$(PLAT)/bin/Bdk-2.0.gir'

include $(top_srcdir)/build/Makefile.msvc-introspection

else
INTROSPECTION_INTERMEDIATE_ITEMS =

endif

dist-hook:	\
	$(top_builddir)/build/win32/vs9/btk.vcproj	\
	$(top_builddir)/build/win32/vs9/btk.headers	\
	$(INTROSPECTION_INTERMEDIATE_ITEMS)

# Install a RC file for the default BTK+ theme, and key themes
install-data-local: install-ms-lib install-def-file install-mac-key-theme
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Raleigh/btk-2.0
	$(INSTALL_DATA) $(srcdir)/btkrc.default $(DESTDIR)$(datadir)/themes/Raleigh/btk-2.0/btkrc
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Default/btk-2.0-key
	$(INSTALL_DATA) $(srcdir)/btkrc.key.default $(DESTDIR)$(datadir)/themes/Default/btk-2.0-key/btkrc
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Emacs/btk-2.0-key
	$(INSTALL_DATA) $(srcdir)/btkrc.key.emacs $(DESTDIR)$(datadir)/themes/Emacs/btk-2.0-key/btkrc

uninstall-local: uninstall-ms-lib uninstall-def-file uninstall-mac-key-theme
	rm -f $(DESTDIR)$(datadir)/themes/Raleigh/btk-2.0/btkrc
	rm -f $(DESTDIR)$(datadir)/themes/Default/btk-2.0-key/btkrc
	rm -f $(DESTDIR)$(datadir)/themes/Emacs/btk-2.0-key/btkrc

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

DEPS = $(btktargetlib) $(top_builddir)/bdk/$(bdktargetlib)

TEST_DEPS = $(DEPS) immodules.cache

LDADDS =								\
	$(btktargetlib)							\
	$(top_builddir)/bdk/$(bdktargetlib)                             \
	$(BTK_DEP_LIBS)

if HAVE_INTROSPECTION
introspection_files = \
    $(btkinclude_HEADERS) \
    $(btk_all_c_sources) \
    btktypebuiltins.h \
    btktypebuiltins.c

Btk-2.0.gir: $(INTROSPECTION_SCANNER) $(btktargetlib) $(top_builddir)/bdk/Bdk-2.0.gir Makefile
Btk_2_0_gir_SCANNERFLAGS = --include-uninstalled=$(top_builddir)/bdk/Bdk-2.0.gir
if USE_X11
Btk_2_0_gir_SCANNERFLAGS += --add-include-path=$(top_builddir)/bdk/x11
endif
Btk_2_0_gir_INCLUDES = Batk-1.0
Btk_2_0_gir_CFLAGS = \
		$(INCLUDES) \
		-UBDK_DISABLE_DEPRECATED \
		-UBTK_DISABLE_DEPRECATED \
		-DBTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
Btk_2_0_gir_LIBS = $(btktargetlib)
Btk_2_0_gir_FILES = $(introspection_files)
Btk_2_0_gir_EXPORT_PACKAGES = btk+-2.0
INTROSPECTION_GIRS += Btk-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/birepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif

#
# Installed tools
#
bin_PROGRAMS = \
	btk-query-immodules-2.0 \
	btk-update-icon-cache

bin_SCRIPTS = btk-builder-convert

if OS_WIN32

# Workaround for UAC silliness: programs with "update" in their name
# are believed to be installers and require elevated privileges to be
# used... Embed a manifest file into executable to tell Windows that
# btk-update-icon-cache.exe doesn't require any special privileges.

BTK_UPDATE_ICON_CACHE_MANIFEST = btk-update-icon-cache.exe.manifest
BTK_UPDATE_ICON_CACHE_RC = btk-update-icon-cache.rc
BTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT = btk-update-icon-cache_manifest.o

$(BTK_UPDATE_ICON_CACHE_MANIFEST):
	(echo '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' ; \
	 echo '<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">' ; \
	 echo '  <assemblyIdentity version="1.0.0.0"' ; \
	 echo '     processorArchitecture="'$(EXE_MANIFEST_ARCHITECTURE)'"' ; \
	 echo '     name="btk-update-icon-cache.exe"' ; \
	 echo '     type="win32"/>' ; \
	 echo '  <!-- Identify the application security requirements. -->' ; \
	 echo '  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">' ; \
	 echo '    <security>' ; \
	 echo '      <requestedPrivileges>' ; \
	 echo '        <requestedExecutionLevel' ; \
	 echo '          level="asInvoker"' ; \
	 echo '          uiAccess="false"/>' ; \
	 echo '        </requestedPrivileges>' ; \
	 echo '       </security>' ; \
	 echo '  </trustInfo>' ; \
	 echo '</assembly>' ) >$@

$(BTK_UPDATE_ICON_CACHE_RC):
	(echo -e '#include <winbase.h>\nCREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST '$(BTK_UPDATE_ICON_CACHE_MANIFEST)) >$@

$(BTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT): $(BTK_UPDATE_ICON_CACHE_RC) $(BTK_UPDATE_ICON_CACHE_MANIFEST)
	$(WINDRES) --input $< --output $@ --output-format=coff

endif

btk_query_immodules_2_0_DEPENDENCIES = $(DEPS)
btk_query_immodules_2_0_LDADD = $(LDADDS) $(BMODULE_LIBS)
btk_query_immodules_2_0_SOURCES = queryimmodules.c

btk_update_icon_cache_DEPENDENCIES = $(BTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT)
btk_update_icon_cache_LDADD = $(BDK_PIXBUF_LIBS) $(BTK_UPDATE_ICON_CACHE_MANIFEST_OBJECT)
btk_update_icon_cache_SOURCES = updateiconcache.c 

.PHONY: files test test-debug

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done

STOCK_ICONS = \
	stock-icons/16/application-exit.png		\
	stock-icons/16/dialog-information.png		\
	stock-icons/16/document-new.png			\
	stock-icons/16/document-open.png		\
	stock-icons/16/document-open-recent.png		\
	stock-icons/16/document-print.png		\
	stock-icons/16/document-print-preview.png	\
	stock-icons/16/document-properties.png		\
	stock-icons/16/document-revert-ltr.png		\
	stock-icons/16/document-revert-rtl.png		\
	stock-icons/16/document-save-as.png		\
	stock-icons/16/drive-harddisk.png		\
	stock-icons/16/edit-clear.png			\
	stock-icons/16/edit-copy.png			\
	stock-icons/16/edit-cut.png			\
	stock-icons/16/edit-delete.png			\
	stock-icons/16/edit-find.png			\
	stock-icons/16/edit-find-replace.png		\
	stock-icons/16/edit-paste.png			\
	stock-icons/16/edit-redo-ltr.png		\
	stock-icons/16/edit-redo-rtl.png		\
	stock-icons/16/edit-select-all.png		\
	stock-icons/16/edit-undo-ltr.png		\
	stock-icons/16/edit-undo-rtl.png		\
	stock-icons/16/folder.png			\
	stock-icons/16/format-indent-less-ltr.png	\
	stock-icons/16/format-indent-less-rtl.png	\
	stock-icons/16/format-indent-more-ltr.png	\
	stock-icons/16/format-indent-more-rtl.png	\
	stock-icons/16/format-justify-center.png	\
	stock-icons/16/format-justify-fill.png		\
	stock-icons/16/format-justify-left.png		\
	stock-icons/16/format-justify-right.png		\
	stock-icons/16/format-text-bold.png		\
	stock-icons/16/format-text-italic.png		\
	stock-icons/16/format-text-strikethrough.png	\
	stock-icons/16/format-text-underline.png	\
	stock-icons/16/go-previous-ltr.png		\
	stock-icons/16/go-bottom.png			\
	stock-icons/16/go-down.png			\
	stock-icons/16/go-first-ltr.png			\
	stock-icons/16/go-home.png			\
	stock-icons/16/go-jump-ltr.png			\
	stock-icons/16/go-jump-rtl.png			\
	stock-icons/16/go-last-ltr.png			\
	stock-icons/16/go-next-ltr.png			\
	stock-icons/16/go-previous-ltr.png		\
	stock-icons/16/go-top.png			\
	stock-icons/16/go-up.png			\
	stock-icons/16/btk-caps-lock-warning.png	\
	stock-icons/16/btk-color-picker.png		\
	stock-icons/16/btk-connect.png			\
	stock-icons/16/btk-convert.png			\
	stock-icons/16/btk-disconnect.png		\
	stock-icons/16/btk-edit.png			\
	stock-icons/16/btk-font.png			\
	stock-icons/16/btk-index.png			\
	stock-icons/16/btk-orientation-landscape.png	\
	stock-icons/16/btk-orientation-portrait.png	\
	stock-icons/16/btk-orientation-reverse-landscape.png \
	stock-icons/16/btk-orientation-reverse-portrait.png \
	stock-icons/16/btk-page-setup.png		\
	stock-icons/16/btk-preferences.png		\
	stock-icons/16/btk-select-color.png		\
	stock-icons/16/btk-select-font.png		\
	stock-icons/16/btk-undelete-ltr.png		\
	stock-icons/16/btk-undelete-rtl.png		\
	stock-icons/16/help-about.png			\
	stock-icons/16/help-contents.png		\
	stock-icons/16/image-missing.png		\
	stock-icons/16/list-add.png			\
	stock-icons/16/list-remove.png			\
	stock-icons/16/media-floppy.png			\
	stock-icons/16/media-optical.png		\
	stock-icons/16/media-playback-pause.png		\
	stock-icons/16/media-playback-start-ltr.png	\
	stock-icons/16/media-playback-start-rtl.png	\
	stock-icons/16/media-playback-stop.png		\
	stock-icons/16/media-record.png			\
	stock-icons/16/media-seek-backward-ltr.png	\
	stock-icons/16/media-seek-forward-ltr.png	\
	stock-icons/16/media-skip-backward-ltr.png	\
	stock-icons/16/media-skip-forward-ltr.png	\
	stock-icons/16/network-idle.png			\
	stock-icons/16/printer-error.png		\
	stock-icons/16/printer-info.png			\
	stock-icons/16/printer-paused.png		\
	stock-icons/16/printer-warning.png		\
	stock-icons/16/process-stop.png			\
	stock-icons/16/system-run.png			\
	stock-icons/16/text-x-generic.png		\
	stock-icons/16/tools-check-spelling.png		\
	stock-icons/16/view-fullscreen.png		\
	stock-icons/16/view-refresh.png			\
	stock-icons/16/view-restore.png			\
	stock-icons/16/view-sort-ascending.png		\
	stock-icons/16/view-sort-descending.png		\
	stock-icons/16/window-close.png			\
	stock-icons/16/zoom-fit-best.png		\
	stock-icons/16/zoom-in.png			\
	stock-icons/16/zoom-original.png		\
	stock-icons/16/zoom-out.png			\
	\
	stock-icons/20/btk-apply.png			\
	stock-icons/20/btk-cancel.png			\
	stock-icons/20/btk-no.png			\
	stock-icons/20/btk-ok.png			\
	stock-icons/20/btk-yes.png			\
	stock-icons/20/window-close.png			\
	\
	stock-icons/24/application-exit.png		\
	stock-icons/24/audio-volume-high.png		\
	stock-icons/24/audio-volume-low.png		\
	stock-icons/24/audio-volume-medium.png		\
	stock-icons/24/audio-volume-muted.png		\
	stock-icons/24/dialog-information.png		\
	stock-icons/24/document-new.png			\
	stock-icons/24/document-open.png		\
	stock-icons/24/document-open-recent.png		\
	stock-icons/24/document-print.png		\
	stock-icons/24/document-print-preview.png	\
	stock-icons/24/document-properties.png		\
	stock-icons/24/document-revert-ltr.png		\
	stock-icons/24/document-revert-rtl.png		\
	stock-icons/24/document-save-as.png		\
	stock-icons/24/drive-harddisk.png		\
	stock-icons/24/edit-clear.png			\
	stock-icons/24/edit-copy.png			\
	stock-icons/24/edit-cut.png			\
	stock-icons/24/edit-delete.png			\
	stock-icons/24/edit-find.png			\
	stock-icons/24/edit-find-replace.png		\
	stock-icons/24/edit-paste.png			\
	stock-icons/24/edit-redo-ltr.png		\
	stock-icons/24/edit-redo-rtl.png		\
	stock-icons/24/edit-select-all.png		\
	stock-icons/24/edit-undo-ltr.png		\
	stock-icons/24/edit-undo-rtl.png		\
	stock-icons/24/folder.png			\
	stock-icons/24/format-indent-less-ltr.png	\
	stock-icons/24/format-indent-less-rtl.png	\
	stock-icons/24/format-indent-more-ltr.png	\
	stock-icons/24/format-indent-more-rtl.png	\
	stock-icons/24/format-justify-center.png	\
	stock-icons/24/format-justify-fill.png		\
	stock-icons/24/format-justify-left.png		\
	stock-icons/24/format-justify-right.png		\
	stock-icons/24/format-text-bold.png		\
	stock-icons/24/format-text-italic.png		\
	stock-icons/24/format-text-strikethrough.png	\
	stock-icons/24/format-text-underline.png	\
	stock-icons/24/go-bottom.png			\
	stock-icons/24/go-down.png			\
	stock-icons/24/go-first-ltr.png			\
	stock-icons/24/go-home.png			\
	stock-icons/24/go-jump-ltr.png			\
	stock-icons/24/go-jump-rtl.png			\
	stock-icons/24/go-last-ltr.png			\
	stock-icons/24/go-next-ltr.png			\
	stock-icons/24/go-previous-ltr.png		\
	stock-icons/24/go-top.png			\
	stock-icons/24/go-up.png			\
	stock-icons/24/btk-caps-lock-warning.png	\
	stock-icons/24/btk-color-picker.png		\
	stock-icons/24/btk-connect.png			\
	stock-icons/24/btk-convert.png			\
	stock-icons/24/btk-disconnect.png		\
	stock-icons/24/btk-edit.png			\
	stock-icons/24/btk-font.png			\
	stock-icons/24/btk-index.png			\
	stock-icons/24/btk-orientation-landscape.png	\
	stock-icons/24/btk-orientation-portrait.png	\
	stock-icons/24/btk-orientation-reverse-landscape.png \
	stock-icons/24/btk-orientation-reverse-portrait.png \
	stock-icons/24/btk-page-setup.png		\
	stock-icons/24/btk-preferences.png		\
	stock-icons/24/btk-select-color.png		\
	stock-icons/24/btk-select-font.png		\
	stock-icons/24/btk-undelete-ltr.png		\
	stock-icons/24/btk-undelete-rtl.png		\
	stock-icons/24/help-about.png			\
	stock-icons/24/help-contents.png		\
	stock-icons/24/image-missing.png		\
	stock-icons/24/list-add.png			\
	stock-icons/24/list-remove.png			\
	stock-icons/24/media-floppy.png			\
	stock-icons/24/media-optical.png		\
	stock-icons/24/media-playback-pause.png		\
	stock-icons/24/media-playback-start-ltr.png	\
	stock-icons/24/media-playback-start-rtl.png	\
	stock-icons/24/media-playback-stop.png		\
	stock-icons/24/media-record.png			\
	stock-icons/24/media-seek-backward-ltr.png	\
	stock-icons/24/media-seek-forward-ltr.png	\
	stock-icons/24/media-skip-backward-ltr.png	\
	stock-icons/24/media-skip-forward-ltr.png	\
	stock-icons/24/network-idle.png			\
	stock-icons/24/printer-error.png		\
	stock-icons/24/printer-info.png			\
	stock-icons/24/printer-paused.png		\
	stock-icons/24/printer-warning.png		\
	stock-icons/24/process-stop.png			\
	stock-icons/24/system-run.png			\
	stock-icons/24/text-x-generic.png		\
	stock-icons/24/tools-check-spelling.png		\
	stock-icons/24/view-fullscreen.png		\
	stock-icons/24/view-refresh.png			\
	stock-icons/24/view-restore.png			\
	stock-icons/24/view-sort-ascending.png		\
	stock-icons/24/view-sort-descending.png		\
	stock-icons/24/window-close.png			\
	stock-icons/24/zoom-fit-best.png		\
	stock-icons/24/zoom-in.png			\
	stock-icons/24/zoom-original.png		\
	stock-icons/24/zoom-out.png			\
	\
	stock-icons/32/btk-dnd-multiple.png		\
	stock-icons/32/btk-dnd.png			\
	\
	stock-icons/48/dialog-error.png			\
	stock-icons/48/dialog-information.png		\
	stock-icons/48/dialog-password.png		\
	stock-icons/48/dialog-question.png		\
	stock-icons/48/dialog-warning.png

GENERATED_ICONS = \
	stock-icons/16/go-previous-rtl.png 		\
	stock-icons/16/go-next-rtl.png 			\
	stock-icons/16/go-first-rtl.png 		\
	stock-icons/16/go-last-rtl.png 			\
	stock-icons/16/media-seek-forward-rtl.png 	\
	stock-icons/16/media-skip-forward-rtl.png 	\
	stock-icons/16/media-skip-backward-rtl.png 	\
	stock-icons/16/media-seek-backward-rtl.png 	\
	stock-icons/16/document-save.png 		\
	stock-icons/16/folder-remote.png 		\
	stock-icons/16/user-home.png 			\
	stock-icons/16/user-desktop.png 		\
	stock-icons/24/go-previous-rtl.png 		\
	stock-icons/24/go-next-rtl.png 			\
	stock-icons/24/go-first-rtl.png 		\
	stock-icons/24/go-last-rtl.png 			\
	stock-icons/24/media-seek-forward-rtl.png 	\
	stock-icons/24/media-skip-forward-rtl.png 	\
	stock-icons/24/media-skip-backward-rtl.png 	\
	stock-icons/24/media-seek-backward-rtl.png 	\
	stock-icons/24/document-save.png 		\
	stock-icons/24/folder-remote.png 		\
	stock-icons/24/user-home.png 			\
	stock-icons/24/user-desktop.png

# need to copy the icons so that we can replace the generated ones with
# symlinks, even in the readonly srcdir case
stamp-icons: $(STOCK_ICONS)
	if [ ! -d stock-icons ]; then 					\
	  for d in 16 20 24 32 48; do 					\
	    mkdir -p stock-icons/$$d; 					\
	  done;								\
	  for f in $(STOCK_ICONS); do					\
	    cp $(srcdir)/$$f $$(dirname $$f);				\
	  done;								\
	  chmod -R u+w stock-icons;					\
        fi &&								\
	for i in 16 24; do						\
	   (cd stock-icons/$$i						\
	   && $(RM) document-save.png 					\
	   && $(LN_S) media-floppy.png document-save.png 		\
	   && $(RM) folder-remote.png 					\
	   && $(LN_S) folder.png folder-remote.png 			\
	   && $(RM) go-first-rtl.png 					\
	   && $(LN_S) go-last-ltr.png go-first-rtl.png 			\
	   && $(RM) go-last-rtl.png 					\
	   && $(LN_S) go-first-ltr.png go-last-rtl.png 			\
	   && $(RM) go-next-rtl.png 					\
	   && $(LN_S) go-previous-ltr.png go-next-rtl.png 		\
	   && $(RM) go-previous-rtl.png 				\
	   && $(LN_S) go-next-ltr.png go-previous-rtl.png 		\
	   && $(RM) media-seek-backward-rtl.png 			\
	   && $(LN_S) media-seek-forward-ltr.png media-seek-backward-rtl.png \
	   && $(RM) media-seek-forward-rtl.png 				\
	   && $(LN_S) media-seek-backward-ltr.png media-seek-forward-rtl.png \
	   && $(RM) media-skip-backward-rtl.png 			\
	   && $(LN_S) media-skip-forward-ltr.png media-skip-backward-rtl.png \
	   && $(RM) media-skip-forward-rtl.png 				\
	   && $(LN_S) media-skip-backward-ltr.png media-skip-forward-rtl.png \
	   && $(RM) user-desktop.png 					\
	   && $(LN_S) folder.png user-desktop.png 			\
	   && $(RM) user-home.png 					\
	   && $(LN_S) folder.png user-home.png 				\
	) done 								\
	&& touch stamp-icons

if CROSS_COMPILING
btk_update_icon_cache_program = $(BTK_UPDATE_ICON_CACHE)
else
btk_update_icon_cache_program =							\
	   ./btk-update-icon-cache
endif

btkbuiltincache.h: @REBUILD@ stamp-icons
	$(MAKE) $(AM_MAKEFLAGS) btk-update-icon-cache$(EXEEXT)
	$(btk_update_icon_cache_program) --force --ignore-theme-index		\
	   --include-image-data							\
	   --source builtin_icons stock-icons > btkbuiltincache.h.tmp &&        \
	mv btkbuiltincache.h.tmp btkbuiltincache.h

EXTRA_DIST +=                   \
	$(STOCK_ICONS)		\
	$(GENERATED_ICONS)	\
	line-arrow.xbm 		\
	line-wrap.xbm 		\
	tree_plus.xbm		\
	tree_minus.xbm          \
	tree_minus.xpm		\
	tree_plus.xpm		\
	btk-builder-convert	\
	btk-win32.rc		\
	btk-win32.rc.in		\
	btkwin32embed.h		\
	btkwin32embedwidget.h	\
	btkwin32embedwidget.c	\
	btkprint-win32.h	\
	btkprint-win32.c	\
	btksearchenginequartz.h \
	btkrc.default		\
	btkrc.key.default	\
	btkrc.key.emacs		\
	btkrc.key.mac		\
	makefile.msc		\
	makefile.msc.in		\
	makebtkalias.pl		\
	aliasfilescheck.sh	\
	abicheck.sh		\
	pltcheck.sh		\
	btktypebuiltins.c.template      \
	btktypebuiltins.h.template


install-data-local:

-include $(top_srcdir)/git.mk
