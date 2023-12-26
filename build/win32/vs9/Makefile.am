include $(top_srcdir)/Makefile.decl

GENERATED_ITEMS = \
	bdk-win32.vcproj	\
	bdk.vcproj	\
	btk.vcproj	\
	bailutil.vcproj	\
	btk-demo.vcproj	\
	libwimp.vcproj	\
	btk-install.vsprops

EXTRA_DIST +=	\
	README.txt	\
	btk+.sln	\
	btk-build-defines.vsprops	\
	btk-gen-srcs.vsprops	\
	btk-install.vspropsin	\
	btk-version-paths.vsprops	\
	bdk-win32.vcprojin	\
	bdk.vcprojin	\
	btk.vcprojin	\
	btk-demo.vcprojin	\
	bailutil.vcprojin	\
	libwimp.vcprojin	\
	btk-update-icon-cache.vcproj	\
	btk-install.vcproj	\
	$(GENERATED_ITEMS)

MSVC_INTERMEDIATE_FILES = bdk-win32.headers bdk.headers btk.headers bailutil.headers

btk-install.vsprops: $(MSVC_INTERMEDIATE_FILES)
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/btk-install.vspropsin >$@
	rm $(MSVC_INTERMEDIATE_FILES)

DISTCLEANFILES = $(GENERATED_ITEMS)

