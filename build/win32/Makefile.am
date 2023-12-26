include $(top_srcdir)/Makefile.decl

if HAVE_INTROSPECTION
GENERATED_ITEMS =		\
	introspection.body.mak	\
	Bdk_2_0_gir_list	\
	Btk_2_0_gir_list

MSVC_INTROSPECTION_INTERMEDIATE_FILES = Bdk-2.0.gir.msvc.introspect Btk-2.0.gir.msvc.introspect

introspection.body.mak: $(MSVC_INTROSPECTION_INTERMEDIATE_FILES)
	-$(RM) introspection.body.mak
	for F in `ls *.msvc.introspect`; do \
		case $$F in \
			*)	cat $(top_builddir)/build/win32/$$F >>introspection.body.mak \
			;; \
		esac; \
	done
	$(RM) $(MSVC_INTROSPECTION_INTERMEDIATE_FILES)

DISTCLEANFILES = $(GENERATED_ITEMS)

else
GENERATED_ITEMS =
DISTCLEANFILES =
endif

SUBDIRS =	\
	vs9	\
	vs10	\
	vs11	\
	vs12	\
	vs14	\
	vs15	\
	vs16

EXTRA_DIST +=	\
	detectenv-msvc.mak		\
	introspection-msvc.mak		\
	btk-introspection-msvc.mak	\
	btkpc.py			\
	pc_base.py			\
	replace.py			\
	$(GENERATED_ITEMS)
