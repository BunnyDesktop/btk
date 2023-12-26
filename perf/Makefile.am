## Makefile.am for btk+/perf
include $(top_srcdir)/Makefile.decl

INCLUDES =				\
	-I$(top_srcdir)			\
	-I$(top_builddir)/bdk		\
	-I$(top_srcdir)/bdk		\
	-DBDK_DISABLE_DEPRECATED	\
	-DBTK_DISABLE_DEPRECATED	\
	$(BTK_DEBUG_FLAGS)		\
	$(BTK_DEP_CFLAGS)

DEPS =									\
	$(top_builddir)/bdk/$(bdktargetlib)				\
	$(top_builddir)/btk/$(btktargetlib)

LDADDS =								\
	$(top_builddir)/bdk/$(bdktargetlib)				\
	$(top_builddir)/btk/$(btktargetlib)				\
	$(BTK_DEP_LIBS)

noinst_PROGRAMS	= 	\
	testperf

testperf_DEPENDENCIES = $(TEST_DEPS)

testperf_LDADD = $(LDADDS)

testperf_SOURCES =		\
	appwindow.c		\
	btkwidgetprofiler.c	\
	btkwidgetprofiler.h	\
	main.c			\
	marshalers.c		\
	marshalers.h		\
	textview.c		\
	treeview.c		\
	typebuiltins.c		\
	typebuiltins.h		\
	widgets.h

BUILT_SOURCES =			\
	marshalers.c		\
	marshalers.h		\
	typebuiltins.c		\
	typebuiltins.h

stamp_files =			\
	stamp-marshalers.h	\
	stamp-typebuiltins.h

headers_with_enums = 		\
	btkwidgetprofiler.h

MAINTAINERCLEANFILES = $(BUILT_SOURCES) $(stamp_files)

marshalers.h: stamp-marshalers.h
	@true

stamp-marshalers.h: @REBUILD@ marshalers.list
	$(BUNNYLIB_GENMARSHAL) --prefix=_btk_marshal $(srcdir)/marshalers.list --header >> xgen-gmlh \
	&& (cmp -s xgen-gmlh marshalers.h || cp xgen-gmlh marshalers.h) \
	&& rm -f xgen-gmlh \
	&& echo timestamp > $(@F)	
marshalers.c: @REBUILD@ marshalers.list
	$(BUNNYLIB_GENMARSHAL) --prefix=_btk_marshal $(srcdir)/marshalers.list --body >> xgen-gmlc \
	&& cp xgen-gmlc marshalers.c \
	&& rm -f xgen-gmlc

typebuiltins.h: stamp-typebuiltins.h
	@true
stamp-typebuiltins.h: @REBUILD@ $(headers_with_enums) typebuiltins.h.template
	( cd $(srcdir) && $(BUNNYLIB_MKENUMS) --template typebuiltins.h.template \
		$(headers_with_enums) ) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh typebuiltins.h || cp xgen-gtbh typebuiltins.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)
typebuiltins.c: @REBUILD@ $(headers_with_enums) typebuiltins.c.template
	( cd $(srcdir) && $(BUNNYLIB_MKENUMS) --template typebuiltins.c.template \
		$(headers_with_enums) ) > xgen-gtbc \
	&& cp xgen-gtbc typebuiltins.c  \
	&& rm -f xgen-gtbc


EXTRA_DIST += 		\
	README		\
	marshalers.list	\
	typebuiltins.c.template	\
	typebuiltins.h.template	\
	$(BUILT_SOURCES)

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

-include $(top_srcdir)/git.mk
