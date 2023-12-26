## Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.decl

AUTOMAKE_OPTIONS = 1.6

# The name of the module.
DOC_MODULE=btk2

# The top-level SGML file.
DOC_MAIN_SGML_FILE=btk-docs.sgml

# Extra options to supply to btkdoc-scan
SCAN_OPTIONS=--deprecated-guards="BTK_ENABLE_BROKEN|BTK_DISABLE_DEPRECATED" 

# Extra options to pass to btkdoc-scangobj
SCANGOBJ_OPTIONS=--type-init-func="btk_type_init(0)" 

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=../../../btk

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/btk/*.h
CFILE_GLOB=$(top_srcdir)/btk/*.c

# Header files to ignore when scanning
IGNORE_HFILES=				\
	btkalias.h			\
	fnmatch.h			\
	btkdebug.h			\
	btkbuilderprivate.h		\
	btkdndcursors.h			\
	btkentryprivate.h		\
	btkfilechooserdefault.h		\
	btkfilechooserembed.h		\
	btkfilechooserentry.h		\
	btkfilechooserprivate.h		\
	btkfilechoosersettings.h	\
	btkfilechooserutils.h		\
	btkfilesystem.h			\
	btkfilesystemmodel.h		\
	btkfilesystemunix.h		\
	btkfilesystemwin32.h		\
	btkiconcache.h			\
        btkiconcachevalidator.h         \
	btkiconthemeparser.h		\
	btkintl.h			\
	btkkeyhash.h			\
	btkmarshal.h			\
	btkmnemonichash.h		\
	btkpathbar.h			\
	btkplugprivate.h		\
	btkprintbackend.h		\
	btkprinteroption.h		\
	btkprinteroptionset.h		\
	btkprinteroptionwidget.h	\
	btkprinter-private.h		\
	btkprint-win32.h		\
	btkprintutils.h			\
	btkprintoperation-private.h	\
	btkprivate.h			\
	btkquery.h			\
	btkrbtree.h			\
	btkrecentchooserdefault.h	\
	btkrecentchooserutils.h		\
	btkrecentchooserprivate.h	\
	btksearchengine.h		\
	btksearchenginebeagle.h		\
	btksearchenginetracker.h	\
	btksearchenginesimple.h		\
	btksearchenginequartz.h		\
	btksequence.h			\
	btksocketprivate.h		\
	btktextbtree.h			\
	btktextbufferserializ.h		\
	btktextchildprivate.h		\
	btktextdisplay.h		\
	btktextiterprivate.h		\
	btktextlayout.h			\
	btktextmarkprivate.h		\
	btktextsegment.h		\
	btktexttagprivate.h		\
	btktexttypes.h			\
	btktextutil.h			\
	btkthemes.h			\
	btktoggleactionprivate.h	\
	btktrayicon.h			\
	btktreedatalist.h		\
	btktreeprivate.h		\
	btktoolpaletteprivate.h		\
	btktypebuiltins.h		\
	btkxembed.h			\
	btkwin32embed.h			\
	btkwin32embedwidget.h		\
	btkwindow-decorate.h		\
	xdgmime				\
	xembed.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
INCLUDES = \
	-I$(top_srcdir) 	\
	-I$(top_builddir) 	\
	-I$(top_builddir)/bdk	\
	-DBTK_ENABLE_BROKEN	\
	$(BTK_DEBUG_FLAGS)	\
	$(BTK_DEP_CFLAGS)

CPPFLAGS += \
	-UBTK_DISABLE_SINGLE_INCLUDES

BTKDOC_LIBS = \
	$(top_builddir)/bdk/$(bdktargetlib)        			\
	$(top_builddir)/btk/$(btktargetlib)


# Extra options to supply to btkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml --name-space=btk

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =					\
	version.xml				\
	running.sgml				\
	building.sgml				\
	changes-1.2.sgml			\
	changes-2.0.sgml			\
	compiling.sgml				\
	directfb.sgml				\
	drawing-model.xml			\
	glossary.xml				\
	migrating-checklist.sgml		\
        migrating-ClientSideWindows.sgml	\
	migrating-BtkAboutDialog.sgml		\
	migrating-BtkAction.sgml		\
	migrating-BtkAssistant.sgml		\
	migrating-BtkBuilder.sgml		\
	migrating-BtkColorButton.sgml		\
	migrating-BtkComboBox.sgml		\
	migrating-BtkEntry-icons.sgml		\
	migrating-BtkFileChooser.sgml		\
	migrating-BtkIconView.sgml		\
	migrating-BtkLabel-links.sgml		\
	migrating-BtkLinkButton.sgml		\
	migrating-BtkRecentChooser.sgml 	\
        migrating-BtkTooltip.sgml		\
	objects_grouped.sgml			\
	osx.sgml				\
	question_index.sgml			\
	resources.sgml				\
	text_widget.sgml			\
	tree_widget.sgml			\
	windows.sgml				\
	x11.sgml				\
	btk-query-immodules-2.0.xml		\
	btk-update-icon-cache.xml		\
	btk-builder-convert.xml			\
	visual_index.xml

expand_content_files = 				\
	drawing-model.xml			\
	glossary.xml				\
	migrating-checklist.sgml		\
        migrating-ClientSideWindows.sgml	\
	migrating-BtkAction.sgml		\
	migrating-BtkComboBox.sgml		\
	migrating-BtkEntry-icons.sgml		\
	migrating-BtkFileChooser.sgml		\
	migrating-BtkIconView.sgml		\
	migrating-BtkAboutDialog.sgml		\
	migrating-BtkColorButton.sgml		\
	migrating-BtkAssistant.sgml		\
	migrating-BtkRecentChooser.sgml 	\
	migrating-BtkLabel-links.sgml		\
	migrating-BtkLinkButton.sgml		\
	migrating-BtkBuilder.sgml		\
	migrating-BtkTooltip.sgml		\
	tree_widget.sgml			\
	text_widget.sgml			\
	question_index.sgml

# Images to copy into HTML directory
HTML_IMAGES = \
	$(top_srcdir)/btk/stock-icons/24/help-about.png			\
	$(top_srcdir)/btk/stock-icons/24/list-add.png			\
	$(top_srcdir)/btk/stock-icons/20/btk-apply.png			\
	$(top_srcdir)/btk/stock-icons/24/format-text-bold.png		\
	$(top_srcdir)/btk/stock-icons/20/btk-cancel.png			\
	$(top_srcdir)/btk/stock-icons/24/btk-caps-lock-warning.png	\
	$(top_srcdir)/btk/stock-icons/24/media-optical.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-clear.png			\
	$(top_srcdir)/btk/stock-icons/24/window-close.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-connect.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-convert.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-copy.png			\
	$(top_srcdir)/btk/stock-icons/24/edit-cut.png			\
	$(top_srcdir)/btk/stock-icons/24/edit-delete.png		\
	$(top_srcdir)/btk/stock-icons/48/dialog-password.png		\
	$(top_srcdir)/btk/stock-icons/48/dialog-error.png		\
	$(top_srcdir)/btk/stock-icons/48/dialog-information.png		\
	$(top_srcdir)/btk/stock-icons/48/dialog-question.png		\
	$(top_srcdir)/btk/stock-icons/48/dialog-warning.png		\
	$(top_srcdir)/btk/stock-icons/24/folder.png			\
	$(top_srcdir)/btk/stock-icons/24/btk-disconnect.png		\
	$(top_srcdir)/btk/stock-icons/32/btk-dnd.png			\
	$(top_srcdir)/btk/stock-icons/32/btk-dnd-multiple.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-edit.png			\
	$(top_srcdir)/btk/stock-icons/24/system-run.png			\
	$(top_srcdir)/btk/stock-icons/24/text-x-generic.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-find.png			\
	$(top_srcdir)/btk/stock-icons/24/edit-find-replace.png		\
	$(top_srcdir)/btk/stock-icons/24/go-bottom.png			\
	$(top_srcdir)/btk/stock-icons/24/go-first-ltr.png		\
	$(top_builddir)/btk/stock-icons/24/go-first-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/go-last-ltr.png		\
	$(top_builddir)/btk/stock-icons/24/go-last-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/go-top.png			\
	$(top_srcdir)/btk/stock-icons/24/go-previous-ltr.png		\
	$(top_builddir)/btk/stock-icons/24/go-previous-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/go-down.png			\
	$(top_srcdir)/btk/stock-icons/24/go-next-ltr.png		\
	$(top_builddir)/btk/stock-icons/24/go-next-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/go-up.png			\
	$(top_srcdir)/btk/stock-icons/24/drive-harddisk.png		\
	$(top_srcdir)/btk/stock-icons/24/network-idle.png		\
	$(top_srcdir)/btk/stock-icons/24/help-contents.png		\
	$(top_srcdir)/btk/stock-icons/24/go-home.png			\
	$(top_srcdir)/btk/stock-icons/24/btk-index.png			\
	$(top_srcdir)/btk/stock-icons/24/format-indent-more-ltr.png	\
	$(top_srcdir)/btk/stock-icons/24/format-indent-more-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/format-indent-less-ltr.png	\
	$(top_srcdir)/btk/stock-icons/24/format-indent-less-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/format-text-italic.png		\
	$(top_srcdir)/btk/stock-icons/24/go-jump-ltr.png		\
	$(top_srcdir)/btk/stock-icons/24/go-jump-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/format-justify-center.png	\
	$(top_srcdir)/btk/stock-icons/24/format-justify-fill.png	\
	$(top_srcdir)/btk/stock-icons/24/format-justify-left.png	\
	$(top_srcdir)/btk/stock-icons/24/format-justify-right.png	\
	$(top_srcdir)/btk/stock-icons/24/image-missing.png		\
	$(top_srcdir)/btk/stock-icons/24/document-new.png		\
	$(top_srcdir)/btk/stock-icons/20/btk-no.png			\
	$(top_srcdir)/btk/stock-icons/20/btk-ok.png			\
	$(top_srcdir)/btk/stock-icons/24/document-open.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-orientation-landscape.png	\
	$(top_srcdir)/btk/stock-icons/24/btk-orientation-portrait.png	\
	$(top_srcdir)/btk/stock-icons/24/btk-orientation-reverse-landscape.png \
	$(top_srcdir)/btk/stock-icons/24/btk-orientation-reverse-portrait.png \
	$(top_srcdir)/btk/stock-icons/24/edit-paste.png			\
	$(top_srcdir)/btk/stock-icons/24/btk-preferences.png		\
	$(top_srcdir)/btk/stock-icons/24/document-print.png		\
	$(top_srcdir)/btk/stock-icons/24/printer-error.png		\
	$(top_srcdir)/btk/stock-icons/24/printer-paused.png		\
	$(top_srcdir)/btk/stock-icons/24/document-print-preview.png	\
	$(top_srcdir)/btk/stock-icons/24/printer-info.png		\
	$(top_srcdir)/btk/stock-icons/24/printer-warning.png		\
	$(top_srcdir)/btk/stock-icons/24/document-properties.png	\
	$(top_srcdir)/btk/stock-icons/24/application-exit.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-redo-ltr.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-redo-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/view-refresh.png		\
	$(top_srcdir)/btk/stock-icons/24/list-remove.png		\
	$(top_srcdir)/btk/stock-icons/24/document-revert-ltr.png	\
	$(top_srcdir)/btk/stock-icons/24/document-revert-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/media-floppy.png		\
	$(top_builddir)/btk/stock-icons/24/document-save.png		\
	$(top_srcdir)/btk/stock-icons/24/document-save-as.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-select-all.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-select-color.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-color-picker.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-font.png			\
	$(top_srcdir)/btk/stock-icons/24/view-sort-ascending.png	\
	$(top_srcdir)/btk/stock-icons/24/view-sort-descending.png	\
	$(top_srcdir)/btk/stock-icons/24/tools-check-spelling.png	\
	$(top_srcdir)/btk/stock-icons/24/process-stop.png		\
	$(top_srcdir)/btk/stock-icons/24/format-text-strikethrough.png	\
	$(top_srcdir)/btk/stock-icons/24/btk-undelete-ltr.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-undelete-rtl.png		\
	$(top_srcdir)/btk/stock-icons/24/format-text-underline.png	\
	$(top_srcdir)/btk/stock-icons/24/edit-undo-ltr.png		\
	$(top_srcdir)/btk/stock-icons/24/edit-undo-rtl.png		\
	$(top_srcdir)/btk/stock-icons/20/btk-yes.png			\
	$(top_srcdir)/btk/stock-icons/24/zoom-original.png		\
	$(top_srcdir)/btk/stock-icons/24/zoom-fit-best.png		\
	$(top_srcdir)/btk/stock-icons/24/zoom-in.png			\
	$(top_srcdir)/btk/stock-icons/24/zoom-out.png			\
	$(top_srcdir)/btk/stock-icons/24/media-seek-forward-ltr.png	\
	$(top_builddir)/btk/stock-icons/24/media-seek-forward-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/media-skip-forward-ltr.png	\
	$(top_builddir)/btk/stock-icons/24/media-skip-forward-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/media-playback-pause.png	\
	$(top_srcdir)/btk/stock-icons/24/media-playback-start-ltr.png	\
	$(top_srcdir)/btk/stock-icons/24/media-playback-start-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/media-seek-backward-ltr.png	\
	$(top_builddir)/btk/stock-icons/24/media-seek-backward-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/media-record.png		\
	$(top_srcdir)/btk/stock-icons/24/media-skip-backward-ltr.png	\
	$(top_builddir)/btk/stock-icons/24/media-skip-backward-rtl.png	\
	$(top_srcdir)/btk/stock-icons/24/media-playback-stop.png	\
	$(top_srcdir)/btk/stock-icons/24/view-fullscreen.png		\
	$(top_srcdir)/btk/stock-icons/24/view-restore.png		\
	$(top_srcdir)/btk/stock-icons/24/btk-page-setup.png		\
	$(srcdir)/images/aboutdialog.png				\
	$(srcdir)/images/accel-label.png				\
	$(srcdir)/images/assistant.png					\
	$(srcdir)/images/button.png					\
	$(srcdir)/images/check-button.png				\
	$(srcdir)/images/color-button.png				\
	$(srcdir)/images/colorsel.png					\
	$(srcdir)/images/combo-box.png					\
	$(srcdir)/images/combo-box-entry.png				\
	$(srcdir)/images/entry.png					\
	$(srcdir)/images/figure-hierarchical-drawing.png		\
	$(srcdir)/images/figure-windowed-label.png			\
	$(srcdir)/images/file-button.png				\
	$(srcdir)/images/filechooser.png				\
	$(srcdir)/images/font-button.png				\
	$(srcdir)/images/fontsel.png					\
	$(srcdir)/images/frame.png					\
	$(srcdir)/images/icon-view.png					\
	$(srcdir)/images/image.png					\
	$(srcdir)/images/label.png					\
	$(srcdir)/images/link-button.png				\
	$(srcdir)/images/list-and-tree.png				\
	$(srcdir)/images/menubar.png					\
	$(srcdir)/images/messagedialog.png				\
	$(srcdir)/images/multiline-text.png				\
	$(srcdir)/images/notebook.png					\
	$(srcdir)/images/panes.png					\
	$(srcdir)/images/pagesetupdialog.png				\
	$(srcdir)/images/printdialog.png				\
	$(srcdir)/images/progressbar.png				\
	$(srcdir)/images/radio-group.png				\
	$(srcdir)/images/recentchooserdialog.png			\
	$(srcdir)/images/scales.png					\
	$(srcdir)/images/scrolledwindow.png				\
	$(srcdir)/images/separator.png					\
	$(srcdir)/images/spinbutton.png					\
	$(srcdir)/images/spinner.png					\
	$(srcdir)/images/statusbar.png					\
	$(srcdir)/images/toggle-button.png				\
	$(srcdir)/images/toolbar.png					\
	$(srcdir)/images/toolpalette.png				\
	$(srcdir)/images/tree-view-coordinates.png			\
	$(srcdir)/images/volumebutton.png				\
	$(srcdir)/images/window.png					\
	$(srcdir)/images/layout-btlr.png				\
	$(srcdir)/images/layout-btrl.png				\
	$(srcdir)/images/layout-lrbt.png				\
	$(srcdir)/images/layout-lrtb.png				\
	$(srcdir)/images/layout-rlbt.png				\
	$(srcdir)/images/layout-rltb.png				\
	$(srcdir)/images/layout-tblr.png				\
	$(srcdir)/images/layout-tbrl.png

# Extra options to supply to btkdoc-fixref
FIXXREF_OPTIONS=--extra-dir=../bdk/html \
	--extra-dir=$(BUNNYLIB_PREFIX)/share/btk-doc/html/bobject \
	--extra-dir=$(BUNNYLIB_PREFIX)/share/btk-doc/html/bunnylib \
	--extra-dir=$(BATK_PREFIX)/share/btk-doc/html/batk \
	--extra-dir=$(BANGO_PREFIX)/share/btk-doc/html/bango \
	--extra-dir=$(BAIRO_PREFIX)/share/btk-doc/html/bairo

include $(top_srcdir)/btk-doc.make

# Other files to distribute
EXTRA_DIST += version.xml.in

########################################################################

if ENABLE_MAN

man_MANS = btk-query-immodules-2.0.1 btk-update-icon-cache.1 btk-builder-convert.1

%.1 : %.xml 
	@XSLTPROC@ -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

BUILT_EXTRA_DIST = $(man_MANS)

DISTCLEANFILES = $(man_MANS)

endif

dist-hook-local: $(BUILT_EXTRA_DIST)
	files='$(BUILT_EXTRA_DIST)';				\
	for f in $$files; do					\
	  if test -f $$f; then d=.; else d=$(srcdir); fi;	\
	  cp $$d/$$f $(distdir) || exit 1; done

-include $(top_srcdir)/git.mk
