## Makefile.am for btk+/demos
include $(top_srcdir)/Makefile.decl

democodedir=$(datadir)/btk-2.0/demo

## These should be in the order you want them to appear in the
## demo app, which means alphabetized by demo title, not filename
demos =						\
	appwindow.c				\
	assistant.c				\
	builder.c				\
	button_box.c				\
	changedisplay.c				\
	clipboard.c				\
	colorsel.c				\
	combobox.c				\
	dialog.c				\
	drawingarea.c				\
	editable_cells.c			\
	entry_buffer.c				\
	entry_completion.c			\
	expander.c  				\
	hypertext.c				\
	iconview.c				\
	iconview_edit.c				\
	images.c				\
	infobar.c				\
	links.c					\
	list_store.c				\
	menus.c					\
	offscreen_window.c			\
	offscreen_window2.c			\
	panes.c					\
	pickers.c				\
	pixbufs.c				\
	printing.c				\
	rotated_text.c				\
	search_entry.c				\
	sizegroup.c				\
	spinner.c				\
	stock_browser.c				\
	textview.c				\
	textscroll.c				\
	toolpalette.c				\
	tree_store.c				\
	ui_manager.c

INCLUDES = \
	-DDEMOCODEDIR="\"$(democodedir)\""	\
	-I$(top_srcdir) 			\
	-I$(top_builddir)/bdk			\
	-DBDK_DISABLE_DEPRECATED		\
	-DBTK_DISABLE_DEPRECATED 		\
	$(BTK_DEBUG_FLAGS)			\
	$(BTK_DEP_CFLAGS)

DEPS = \
	$(top_builddir)/bdk/$(bdktargetlib) 				\
	$(top_builddir)/btk/$(btktargetlib)

LDADDS = \
	$(top_builddir)/bdk/$(bdktargetlib)        			\
	$(top_builddir)/btk/$(btktargetlib)				\
	$(BTK_DEP_LIBS) 						\
	-lm

bin_PROGRAMS = btk-demo

BUILT_SOURCES = demos.h

EXTRA_DIST += 		\
	$(IMAGEFILES)	\
	demo.ui

demos.h: @REBUILD@ $(demos) geninclude.pl
	(here=`pwd` ; cd $(srcdir) && $(PERL) $$here/geninclude.pl $(demos)) > demos.h

btk_demo_SOURCES = 		\
	$(demos)		\
	demo-common.h		\
	main.c			\
	demos.h

btk_demo_DEPENDENCIES = $(DEPS)
btk_demo_LDADD = $(LDADDS)
btk_demo_LDFLAGS = -export-dynamic

IMAGEFILES=	alphatest.png		\
	        apple-red.png		\
		background.jpg		\
		floppybuddy.gif		\
		gnome-applets.png	\
		gnome-calendar.png	\
		gnome-fs-directory.png	\
		gnome-fs-regular.png	\
		gnome-foot.png		\
		gnome-gimp.png		\
		gnome-gmush.png		\
		gnome-gsame.png		\
		gnu-keys.png		\
		btk-logo-rgb.gif		

democode_DATA = $(demos) $(IMAGEFILES) demo.ui

# ------------------- MSVC Projects --------------------
MSVCPROJS = btk-demo

btk_demo_FILES = $(btk_demo_SOURCES)
btk_demo_EXCLUDES = btkdemodummy

include $(top_srcdir)/build/Makefile.msvcproj

dist-hook: $(top_builddir)/build/win32/vs9/btk-demo.vcproj

DISTCLEANFILES = demos.h

-include $(top_srcdir)/git.mk
