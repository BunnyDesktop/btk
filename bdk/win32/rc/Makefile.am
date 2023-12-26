include $(top_srcdir)/Makefile.decl

EXTRA_DIST += \
	bdk.rc \
	bdk.rc.in \
	btk.ico

if USE_WIN32
noinst_DATA = bdk-win32-res.o
endif

bdk-win32-res.o : bdk.rc btk.ico
	$(WINDRES) bdk.rc $@

-include $(top_srcdir)/git.mk
