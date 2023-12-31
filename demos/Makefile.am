## Makefile.am for btk+/demos
include $(top_srcdir)/Makefile.decl

SUBDIRS = btk-demo

INCLUDES = 				\
	-I$(top_srcdir) 		\
	-I$(top_builddir)/bdk		\
	-DBDK_DISABLE_DEPRECATED	\
	-DBTK_DISABLE_DEPRECATED	\
	$(BTK_DEBUG_FLAGS)		\
	$(BTK_DEP_CFLAGS)

DEPS = 									\
	$(top_builddir)/bdk/$(bdktargetlib) 				\
	$(top_builddir)/btk/$(btktargetlib)

LDADDS = 								\
	$(top_builddir)/bdk/$(bdktargetlib)        			\
	$(top_builddir)/btk/$(btktargetlib)				\
	$(BTK_DEP_LIBS)							\
	$(MATH_LIB)

noinst_PROGRAMS = 		\
	testpixbuf-drawable 	\
	testanimation 		\
	testpixbuf-color	\
	testpixbuf-save		\
	testpixbuf-scale 	\
	pixbuf-demo

# Need to build test-inline-pixbufs.h for testpixbuf
noinst_PROGRAMS += testpixbuf
BUILT_SOURCES = test-inline-pixbufs.h

test-inline-pixbufs.h: apple-red.png bunny-foot.png
	$(BDK_PIXBUF_CSOURCE) --raw --build-list		\
	        apple_red  $(srcdir)/apple-red.png	\
                bunny_foot $(srcdir)/bunny-foot.png	\
        > test-inline-pixbufs.h				\
	|| (rm -f test-inline-pixbufs.h && false)

testpixbuf_DEPENDENCIES = $(DEPS)
testpixbuf_drawable_DEPENDENCIES = $(DEPS)
testpixbuf_save_DEPENDENCIES = $(DEPS)
testpixbuf_color_DEPENDENCIES = $(DEPS)
testpixbuf_scale_DEPENDENCIES = $(DEPS)
testanimation_DEPENDENCIES = $(DEPS)
pixbuf_demo_DEPENDENCIES = $(DEPS)

testpixbuf_LDADD = $(LDADDS)
testpixbuf_drawable_LDADD = $(LDADDS)
testpixbuf_save_LDADD = $(LDADDS)
testpixbuf_color_LDADD = $(LDADDS)
testpixbuf_scale_LDADD = $(LDADDS)
testanimation_LDADD = $(LDADDS)
pixbuf_demo_LDADD = $(LDADDS)

testpixbuf_SOURCES = testpixbuf.c pixbuf-init.c
testpixbuf_drawable_SOURCES = testpixbuf-drawable.c pixbuf-init.c
testpixbuf_save_SOURCES = testpixbuf-save.c
testpixbuf_color_SOURCES = testpixbuf-color.c
testpixbuf_scale_SOURCES = testpixbuf-scale.c pixbuf-init.c
testanimation_SOURCES = testanimation.c pixbuf-init.c
pixbuf_demo_SOURCES = pixbuf-demo.c pixbuf-init.c

EXTRA_DIST +=			\
	apple-red.png		\
	background.jpg		\
	bunny-applets.png	\
	bunny-calendar.png	\
	bunny-foot.png		\
	bunny-gimp.png		\
	bunny-gmush.png		\
	bunny-gsame.png		\
	gnu-keys.png

DISTCLEANFILES = test-inline-pixbufs.h

-include $(top_srcdir)/git.mk
