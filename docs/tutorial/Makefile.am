include $(top_srcdir)/Makefile.decl

.PHONY: html pdf

EXTRA_DIST += \
	btk-tut.sgml			\
	images/arrow.png		\
	images/aspectframe.png		\
	images/base.png			\
	images/buttonbox.png		\
	images/buttons.png		\
	images/calendar.png		\
	images/colorsel.png		\
	images/entry.png		\
	images/eventbox.png		\
	images/filesel.png		\
	images/fixed.png		\
	images/frame.png		\
	images/btkdial.png		\
	images/helloworld.png		\
	images/helloworld2.png		\
	images/label.png		\
	images/menu.png			\
	images/notebook.png		\
	images/packbox1.png		\
	images/packbox2.png		\
	images/paned.png		\
	images/progressbar.png		\
	images/radiobuttons.png		\
	images/rangewidgets.png		\
	images/rulers.png		\
	images/scribble.png		\
	images/scrolledwin.png		\
	images/spinbutton.png		\
	images/statusbar.png		\
	images/table.png		\
	images/tictactoe.png

if HAVE_DOCBOOK
html: btk-tut.sgml
	if test -w $(srcdir); then \
	  (cd $(srcdir); \
	   db2html btk-tut.sgml; \
	   test -d html && rm -r html; \
	   mv btk-tut html; \
	   mkdir html/images; \
	   cp images/*.png html/images); \
	fi

pdf: btk-tut.sgml
	if test -w $(srcdir); then \
	  (cd $(srcdir); db2pdf btk-tut.sgml); \
	fi

all-local: html

dist-hook: html
	cp -Rp $(srcdir)/html $(distdir)
else
html:
	echo "***"
	echo "*** Warning: Tutorial not built"
	echo "***"

pdf:
	echo "***"
	echo "*** Warning: Tutorial not built"
	echo "***"

dist-hook:
	echo "***"
	echo "*** Warning: Tutorial not built"
	echo "*** DISTRIBUTION IS INCOMPLETE"
	echo "***"
endif


-include $(top_srcdir)/git.mk
