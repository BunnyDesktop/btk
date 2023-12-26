include $(top_srcdir)/Makefile.decl

SUBDIRS = libbail-util tests

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/btk-2.0/modules
module_LTLIBRARIES = libbail.la

bail_c_sources =			\
	bail-private-macros.h		\
	bail.c				\
	bailadjustment.c		\
	bailarrow.c			\
	bailbooleancell.c		\
	bailbox.c			\
	bailbutton.c			\
	bailcalendar.c			\
	bailcell.c			\
	bailcellparent.c		\
	bailcheckmenuitem.c		\
	bailchecksubmenuitem.c		\
	bailclist.c			\
	bailclistcell.c			\
	bailcombo.c			\
	bailcombobox.c			\
	bailcontainer.c			\
	bailcontainercell.c		\
	bailentry.c			\
	bailexpander.c			\
	bailframe.c			\
	bailimage.c			\
	bailimagecell.c			\
	bailitem.c			\
	baillabel.c			\
	baillist.c			\
	bailmenu.c			\
	bailmenushell.c			\
	bailmenuitem.c			\
	bailnotebook.c			\
	bailnotebookpage.c		\
	bailobject.c			\
	bailoptionmenu.c		\
	bailpaned.c			\
	bailpixmap.c			\
	bailprogressbar.c		\
	bailradiobutton.c		\
	bailradiomenuitem.c		\
	bailradiosubmenuitem.c		\
	bailrange.c			\
	bailrenderercell.c		\
	bailscale.c			\
	bailscalebutton.c		\
	bailscrollbar.c			\
	bailscrolledwindow.c		\
	bailseparator.c			\
	bailspinbutton.c		\
	bailsubmenuitem.c		\
	bailstatusbar.c			\
	bailtextcell.c			\
	bailtextview.c			\
	bailtogglebutton.c		\
	bailtoplevel.c			\
	bailtreeview.c			\
	bailutil.c			\
	bailwidget.c			\
	bailwindow.c

libbailincludedir=$(includedir)/bail-1.0/bail

bail_private_h_sources =		\
	bail.h				\
	bailadjustment.h		\
	bailarrow.h			\
	bailbooleancell.h		\
	bailbox.h			\
	bailbutton.h			\
	bailcalendar.h			\
	bailcell.h			\
	bailcellparent.h		\
	bailcheckmenuitem.h		\
	bailchecksubmenuitem.h		\
	bailclist.h			\
	bailclistcell.h			\
	bailcombo.h			\
	bailcombobox.h			\
	bailcontainercell.h		\
	bailcontainer.h			\
	bailentry.h			\
	bailexpander.h			\
	bailfactory.h			\
	bailframe.h			\
	bailimage.h			\
	bailimagecell.h			\
	bailitem.h			\
	baillabel.h			\
	baillist.h			\
	bailmenu.h			\
	bailmenushell.h			\
	bailmenuitem.h			\
	bailnotebook.h			\
	bailnotebookpage.h		\
	bailobject.h			\
	bailoptionmenu.h		\
	bailpaned.h			\
	bailpixmap.h			\
	bailprogressbar.h		\
	bailradiobutton.h		\
	bailradiomenuitem.h		\
	bailradiosubmenuitem.h		\
	bailrange.h			\
	bailrenderercell.h		\
	bailscale.h			\
	bailscalebutton.h		\
	bailscrollbar.h			\
	bailscrolledwindow.h		\
	bailseparator.h			\
	bailspinbutton.h		\
	bailsubmenuitem.h		\
	bailstatusbar.h			\
	bailtextcell.h			\
	bailtextview.h			\
	bailtogglebutton.h		\
	bailtoplevel.h			\
	bailtreeview.h			\
	bailutil.h			\
	bailwindow.h

bail_public_h_sources =	\
	bailwidget.h

libbail_la_SOURCES = 			\
	$(bail_c_sources)		\
	$(bail_public_h_sources)	\
	$(bail_private_h_sources)

libbailinclude_HEADERS =		\
	$(bail_public_h_sources)

libbail_la_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/modules/other	\
	-I$(top_srcdir)/bdk		\
	-I$(top_builddir)/bdk		\
	-I$(top_srcdir)/btk		\
	-I$(top_builddir)/btk		\
	-DBTK_VERSION=\"$(BTK_VERSION)\"\
	-DBDK_DISABLE_DEPRECATED	\
	-DBTK_DISABLE_DEPRECATED	\
	$(AM_CPPFLAGS)

if PLATFORM_WIN32
WIN32_ADDITIONAL_CFLAGS = $(BMODULE_CFLAGS)
WIN32_ADDITIONAL_LIBS = $(BMODULE_LIBS)
endif

libbail_la_CFLAGS = \
	$(BTK_DEP_CFLAGS)	\
	$(BTK_DEBUG_FLAGS)	\
	$(WIN32_ADDITIONAL_CFLAGS)		\
	$(AM_CFLAGS)

libbail_la_LIBADD =  \
	$(top_builddir)/bdk/$(bdktargetlib) 				\
	$(top_builddir)/btk/$(btktargetlib)				\
	$(top_builddir)/modules/other/bail/libbail-util/libbailutil.la	\
	$(BTK_DEP_LIBS)							\
	$(WIN32_ADDITIONAL_LIBS)		\
	$(INTLLIBS)

libbail_la_LDFLAGS =    \
	-rpath $(moduledir) -module -avoid-version \
	$(no_undefined) \
        $(LDFLAGS)

-include $(top_srcdir)/git.mk
