include $(top_srcdir)/Makefile.decl

INCLUDES = 						\
	-I$(top_srcdir) 				\
	-I$(top_builddir)/bdk				\
	-I$(top_srcdir)/bdk				\
	-I$(top_srcdir)/bdk/x11				\
	$(BTK_DEBUG_FLAGS)				\
	$(BTK_DEP_CFLAGS)

DEPS = 									\
	$(top_builddir)/bdk/$(bdktargetlib) 				\
	$(top_builddir)/btk/$(btktargetlib)

LDADDS = 								\
	$(top_builddir)/bdk/$(bdktargetlib)				\
	$(top_builddir)/btk/$(btktargetlib)				\
	$(BTK_DEP_LIBS)							\
	$(BDK_DEP_LIBS)							\
	-lm

if USE_X11
noinst_PROGRAMS = 		\
	doc-shooter
endif

doc_shooter_DEPENDENCIES = $(DEPS)
doc_shooter_LDADD = $(LDADDS)
doc_shooter_SOURCES=	\
	shadow.c	\
	shadow.h	\
	shooter.c	\
	widgets.c	\
	widgets.h

clean-local:
	for file in *.png; do \
	  case "$$file" in \
	    folder.png|gnome.png) ;; \
	    *) rm -f $$file ;; \
	  esac ; \
	done

-include $(top_srcdir)/git.mk
