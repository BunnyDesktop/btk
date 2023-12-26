include $(top_srcdir)/Makefile.decl

GENERATED_ITEMS = \
	bdk-win32.vcxproj	\
	bdk.vcxproj		\
	btk.vcxproj		\
	bailutil.vcxproj	\
	libwimp.vcxproj		\
	btk-demo.vcxproj	\
	bdk-win32.vcxproj.filters	\
	bdk.vcxproj.filters	\
	btk.vcxproj.filters	\
	bailutil.vcxproj.filters	\
	libwimp.vcxproj.filters	\
	btk-demo.vcxproj.filters	\
	btk-install.props

EXTRA_DIST +=	\
	README.txt	\
	btk+.sln	\
	btk-build-defines.props	\
	btk-gen-srcs.props	\
	btk-install.propsin	\
	btk-version-paths.props	\
	bdk-win32.vcxprojin	\
	bdk-win32.vcxproj.filtersin	\
	bdk.vcxprojin	\
	bdk.vcxproj.filtersin	\
	btk.vcxprojin	\
	btk.vcxproj.filtersin	\
	btk-demo.vcxprojin	\
	btk-demo.vcxproj.filtersin	\
	bailutil.vcxprojin	\
	bailutil.vcxproj.filtersin	\
	libwimp.vcxprojin	\
	libwimp.vcxproj.filtersin	\
	btk-update-icon-cache.vcxproj	\
	btk-update-icon-cache.vcxproj.filters	\
	btk-install.vcxproj	\
	btk-install.vcxproj.filters	\
	$(GENERATED_ITEMS)

MSVC_INTERMEDIATE_FILES = bdk-win32.vs10.headers bdk.vs10.headers btk.vs10.headers bailutil.vs10.headers

btk-install.props: $(MSVC_INTERMEDIATE_FILES)
	-$(RM) $(top_builddir)/build/win32/vs11/btk-install.props
	-$(RM) $(top_builddir)/build/win32/vs12/btk-install.props
	-$(RM) $(top_builddir)/build/win32/vs14/btk-install.props
	-$(RM) $(top_builddir)/build/win32/vs15/btk-install.props
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/btk-install.propsin >$@
	rm $(MSVC_INTERMEDIATE_FILES)

DISTCLEANFILES = $(GENERATED_ITEMS)
