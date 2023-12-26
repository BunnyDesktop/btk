# -*- mode: makefile -*-
#
# btk-doc.make - make rules for btk-doc
# Copyright (C) 2003 James Henstridge
#               2004-2007 Damon Chaplin
#               2007-2017 Stefan Sauer
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

####################################
# Everything below here is generic #
####################################

if BTK_DOC_USE_LIBTOOL
BTKDOC_CC = $(LIBTOOL) --tag=CC --mode=compile $(CC) $(INCLUDES) $(BTKDOC_DEPS_CFLAGS) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
BTKDOC_LD = $(LIBTOOL) --tag=CC --mode=link $(CC) $(BTKDOC_DEPS_LIBS) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS)
BTKDOC_RUN = $(LIBTOOL) --mode=execute
else
BTKDOC_CC = $(CC) $(INCLUDES) $(BTKDOC_DEPS_CFLAGS) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
BTKDOC_LD = $(CC) $(BTKDOC_DEPS_LIBS) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS)
BTKDOC_RUN =
endif

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH, when it comes to
# whether a source that is a target of one rule is then
# searched for in VPATH/GPATH.
#
GPATH = $(srcdir)

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

SETUP_FILES = \
	$(content_files)		\
	$(expand_content_files)		\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE)-sections.txt	\
	$(DOC_MODULE)-overrides.txt

EXTRA_DIST = 				\
	$(HTML_IMAGES)			\
	$(SETUP_FILES)

DOC_STAMPS=setup-build.stamp scan-build.stamp sgml-build.stamp \
	html-build.stamp pdf-build.stamp \
	sgml.stamp html.stamp pdf.stamp

SCANOBJ_FILES = 		 \
	$(DOC_MODULE).args 	 \
	$(DOC_MODULE).hierarchy  \
	$(DOC_MODULE).interfaces \
	$(DOC_MODULE).prerequisites \
	$(DOC_MODULE).signals

REPORT_FILES = \
	$(DOC_MODULE)-undocumented.txt \
	$(DOC_MODULE)-undeclared.txt \
	$(DOC_MODULE)-unused.txt

btkdoc-check.test: Makefile
	$(AM_V_GEN)echo "#!/bin/sh -e" > $@; \
		echo "$(BTKDOC_CHECK_PATH) || exit 1" >> $@; \
		chmod +x $@

CLEANFILES = $(SCANOBJ_FILES) $(REPORT_FILES) $(DOC_STAMPS) btkdoc-check.test

if BTK_DOC_BUILD_HTML
HTML_BUILD_STAMP=html-build.stamp
else
HTML_BUILD_STAMP=
endif
if BTK_DOC_BUILD_PDF
PDF_BUILD_STAMP=pdf-build.stamp
else
PDF_BUILD_STAMP=
endif

all-btk-doc: $(HTML_BUILD_STAMP) $(PDF_BUILD_STAMP)
.PHONY: all-btk-doc

if ENABLE_BTK_DOC
all-local: all-btk-doc
endif

docs: $(HTML_BUILD_STAMP) $(PDF_BUILD_STAMP)

$(REPORT_FILES): sgml-build.stamp

#### setup ####

BTK_DOC_V_SETUP=$(BTK_DOC_V_SETUP_@AM_V@)
BTK_DOC_V_SETUP_=$(BTK_DOC_V_SETUP_@AM_DEFAULT_V@)
BTK_DOC_V_SETUP_0=@echo "  DOC   Preparing build";

setup-build.stamp:
	-$(BTK_DOC_V_SETUP)if test "$(abs_srcdir)" != "$(abs_builddir)" ; then \
	  files=`echo $(SETUP_FILES) $(DOC_MODULE).types`; \
	  if test "x$$files" != "x" ; then \
	    for file in $$files ; do \
	      destdir=`dirname $(abs_builddir)/$$file`; \
	      test -d "$$destdir" || mkdir -p "$$destdir"; \
	      test -f $(abs_srcdir)/$$file && \
	        cp -pf $(abs_srcdir)/$$file $(abs_builddir)/$$file || true; \
	    done; \
	  fi; \
	fi
	$(AM_V_at)touch setup-build.stamp

#### scan ####

BTK_DOC_V_SCAN=$(BTK_DOC_V_SCAN_@AM_V@)
BTK_DOC_V_SCAN_=$(BTK_DOC_V_SCAN_@AM_DEFAULT_V@)
BTK_DOC_V_SCAN_0=@echo "  DOC   Scanning header files";

BTK_DOC_V_INTROSPECT=$(BTK_DOC_V_INTROSPECT_@AM_V@)
BTK_DOC_V_INTROSPECT_=$(BTK_DOC_V_INTROSPECT_@AM_DEFAULT_V@)
BTK_DOC_V_INTROSPECT_0=@echo "  DOC   Introspecting bobjects";

scan-build.stamp: setup-build.stamp $(HFILE_GLOB) $(CFILE_GLOB)
	$(BTK_DOC_V_SCAN)_source_dir='' ; \
	for i in $(DOC_SOURCE_DIR) ; do \
	  _source_dir="$${_source_dir} --source-dir=$$i" ; \
	done ; \
	btkdoc-scan --module=$(DOC_MODULE) --ignore-headers="$(IGNORE_HFILES)" $${_source_dir} $(SCAN_OPTIONS) $(EXTRA_HFILES)
	$(BTK_DOC_V_INTROSPECT)if grep -l '^..*$$' $(DOC_MODULE).types > /dev/null 2>&1 ; then \
	  scanobj_options=""; \
	  btkdoc-scangobj 2>&1 --help | grep  >/dev/null "\-\-verbose"; \
	  if test "$$?" = "0"; then \
	    if test "x$(V)" = "x1"; then \
	      scanobj_options="--verbose"; \
	    fi; \
	  fi; \
	  CC="$(BTKDOC_CC)" LD="$(BTKDOC_LD)" RUN="$(BTKDOC_RUN)" CFLAGS="$(BTKDOC_CFLAGS) $(CFLAGS)" LDFLAGS="$(BTKDOC_LIBS) $(LDFLAGS)" \
	  btkdoc-scangobj $(SCANGOBJ_OPTIONS) $$scanobj_options --module=$(DOC_MODULE); \
	else \
	  for i in $(SCANOBJ_FILES) ; do \
	    test -f $$i || touch $$i ; \
	  done \
	fi
	$(AM_V_at)touch scan-build.stamp

$(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt: scan-build.stamp
	@true

#### xml ####

BTK_DOC_V_XML=$(BTK_DOC_V_XML_@AM_V@)
BTK_DOC_V_XML_=$(BTK_DOC_V_XML_@AM_DEFAULT_V@)
BTK_DOC_V_XML_0=@echo "  DOC   Building XML";

sgml-build.stamp: setup-build.stamp $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(HFILE_GLOB) $(CFILE_GLOB) $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt $(expand_content_files) xml/btkdocentities.ent
	$(BTK_DOC_V_XML)_source_dir='' ; \
	for i in $(DOC_SOURCE_DIR) ; do \
	  _source_dir="$${_source_dir} --source-dir=$$i" ; \
	done ; \
	btkdoc-mkdb --module=$(DOC_MODULE) --output-format=xml --expand-content-files="$(expand_content_files)" --main-sgml-file=$(DOC_MAIN_SGML_FILE) $${_source_dir} $(MKDB_OPTIONS)
	$(AM_V_at)touch sgml-build.stamp

sgml.stamp: sgml-build.stamp
	@true

$(DOC_MAIN_SGML_FILE): sgml-build.stamp
	@true

xml/btkdocentities.ent: Makefile
	$(BTK_DOC_V_XML)$(MKDIR_P) $(@D) && ( \
		echo "<!ENTITY package \"$(PACKAGE)\">"; \
		echo "<!ENTITY package_bugreport \"$(PACKAGE_BUGREPORT)\">"; \
		echo "<!ENTITY package_name \"$(PACKAGE_NAME)\">"; \
		echo "<!ENTITY package_string \"$(PACKAGE_STRING)\">"; \
		echo "<!ENTITY package_tarname \"$(PACKAGE_TARNAME)\">"; \
		echo "<!ENTITY package_url \"$(PACKAGE_URL)\">"; \
		echo "<!ENTITY package_version \"$(PACKAGE_VERSION)\">"; \
	) > $@

#### html ####

BTK_DOC_V_HTML=$(BTK_DOC_V_HTML_@AM_V@)
BTK_DOC_V_HTML_=$(BTK_DOC_V_HTML_@AM_DEFAULT_V@)
BTK_DOC_V_HTML_0=@echo "  DOC   Building HTML";

BTK_DOC_V_XREF=$(BTK_DOC_V_XREF_@AM_V@)
BTK_DOC_V_XREF_=$(BTK_DOC_V_XREF_@AM_DEFAULT_V@)
BTK_DOC_V_XREF_0=@echo "  DOC   Fixing cross-references";

html-build.stamp: sgml.stamp $(DOC_MAIN_SGML_FILE) $(content_files) $(expand_content_files)
	$(BTK_DOC_V_HTML)rm -rf html && mkdir html && \
	mkhtml_options=""; \
	btkdoc-mkhtml 2>&1 --help | grep  >/dev/null "\-\-verbose"; \
	if test "$$?" = "0"; then \
	  if test "x$(V)" = "x1"; then \
	    mkhtml_options="$$mkhtml_options --verbose"; \
	  fi; \
	fi; \
	btkdoc-mkhtml 2>&1 --help | grep  >/dev/null "\-\-path"; \
	if test "$$?" = "0"; then \
	  mkhtml_options="$$mkhtml_options --path=\"$(abs_srcdir)\""; \
	fi; \
	cd html && btkdoc-mkhtml $$mkhtml_options $(MKHTML_OPTIONS) $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	-@test "x$(HTML_IMAGES)" = "x" || \
	for file in $(HTML_IMAGES) ; do \
	  test -f $(abs_srcdir)/$$file && cp $(abs_srcdir)/$$file $(abs_builddir)/html; \
	  test -f $(abs_builddir)/$$file && cp $(abs_builddir)/$$file $(abs_builddir)/html; \
	  test -f $$file && cp $$file $(abs_builddir)/html; \
	done;
	$(BTK_DOC_V_XREF)btkdoc-fixxref --module=$(DOC_MODULE) --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	$(AM_V_at)touch html-build.stamp

#### pdf ####

BTK_DOC_V_PDF=$(BTK_DOC_V_PDF_@AM_V@)
BTK_DOC_V_PDF_=$(BTK_DOC_V_PDF_@AM_DEFAULT_V@)
BTK_DOC_V_PDF_0=@echo "  DOC   Building PDF";

pdf-build.stamp: sgml.stamp $(DOC_MAIN_SGML_FILE) $(content_files) $(expand_content_files)
	$(BTK_DOC_V_PDF)rm -f $(DOC_MODULE).pdf && \
	mkpdf_options=""; \
	btkdoc-mkpdf 2>&1 --help | grep  >/dev/null "\-\-verbose"; \
	if test "$$?" = "0"; then \
	  if test "x$(V)" = "x1"; then \
	    mkpdf_options="$$mkpdf_options --verbose"; \
	  fi; \
	fi; \
	if test "x$(HTML_IMAGES)" != "x"; then \
	  for img in $(HTML_IMAGES); do \
	    part=`dirname $$img`; \
	    echo $$mkpdf_options | grep >/dev/null "\-\-imgdir=$$part "; \
	    if test $$? != 0; then \
	      mkpdf_options="$$mkpdf_options --imgdir=$$part"; \
	    fi; \
	  done; \
	fi; \
	btkdoc-mkpdf --path="$(abs_srcdir)" $$mkpdf_options $(DOC_MODULE) $(DOC_MAIN_SGML_FILE) $(MKPDF_OPTIONS)
	$(AM_V_at)touch pdf-build.stamp

##############

clean-local:
	@rm -f *~ *.bak
	@rm -rf .libs
	@if echo $(SCAN_OPTIONS) | grep -q "\-\-rebuild-types" ; then \
	  rm -f $(DOC_MODULE).types; \
	fi
	@if echo $(SCAN_OPTIONS) | grep -q "\-\-rebuild-sections" ; then \
	  rm -f $(DOC_MODULE)-sections.txt; \
	fi

distclean-local:
	@rm -rf xml html $(REPORT_FILES) $(DOC_MODULE).pdf \
	    $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt
	@if test "$(abs_srcdir)" != "$(abs_builddir)" ; then \
	    rm -f $(SETUP_FILES) $(DOC_MODULE).types; \
	fi

maintainer-clean-local:
	@rm -rf xml html

install-data-local:
	@installfiles=`echo $(builddir)/html/*`; \
	if test "$$installfiles" = '$(builddir)/html/*'; \
	then echo 1>&2 'Nothing to install' ; \
	else \
	  if test -n "$(DOC_MODULE_VERSION)"; then \
	    installdir="$(DESTDIR)$(TARGET_DIR)-$(DOC_MODULE_VERSION)"; \
	  else \
	    installdir="$(DESTDIR)$(TARGET_DIR)"; \
	  fi; \
	  $(mkinstalldirs) $${installdir} ; \
	  for i in $$installfiles; do \
	    echo ' $(INSTALL_DATA) '$$i ; \
	    $(INSTALL_DATA) $$i $${installdir}; \
	  done; \
	  if test -n "$(DOC_MODULE_VERSION)"; then \
	    mv -f $${installdir}/$(DOC_MODULE).devhelp2 \
	      $${installdir}/$(DOC_MODULE)-$(DOC_MODULE_VERSION).devhelp2; \
	  fi; \
	  $(BTKDOC_REBASE) --relative --dest-dir=$(DESTDIR) --html-dir=$${installdir}; \
	fi

uninstall-local:
	@if test -n "$(DOC_MODULE_VERSION)"; then \
	  installdir="$(DESTDIR)$(TARGET_DIR)-$(DOC_MODULE_VERSION)"; \
	else \
	  installdir="$(DESTDIR)$(TARGET_DIR)"; \
	fi; \
	rm -rf $${installdir}

#
# Require btk-doc when making dist
#
if HAVE_BTK_DOC
dist-check-btkdoc: docs
else
dist-check-btkdoc:
	@echo "*** btk-doc is needed to run 'make dist'.         ***"
	@echo "*** btk-doc was not found when 'configure' ran.   ***"
	@echo "*** please install btk-doc and rerun 'configure'. ***"
	@false
endif

dist-hook: dist-check-btkdoc all-btk-doc dist-hook-local
	@mkdir $(distdir)/html
	@cp ./html/* $(distdir)/html
	@-cp ./$(DOC_MODULE).pdf $(distdir)/
	@-cp ./$(DOC_MODULE).types $(distdir)/
	@-cp ./$(DOC_MODULE)-sections.txt $(distdir)/
	@cd $(distdir) && rm -f $(DISTCLEANFILES)
	@$(BTKDOC_REBASE) --online --relative --html-dir=$(distdir)/html

.PHONY : dist-hook-local docs
