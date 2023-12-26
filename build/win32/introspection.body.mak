vs$(VSVER)\$(CFG)\$(PLAT)\bin\Bdk-2.0.gir: Bdk_2_0_gir_list 
	@-echo Generating $@...
	$(PYTHON) $(G_IR_SCANNER)	\
	--verbose -no-libtool	\
	--namespace=Bdk	\
	--nsversion=2.0	\
		\
	--library=bdk-win32-2.0	\
		\
	--add-include-path=./vs$(VSVER)/$(CFG)/$(PLAT)/bin	\
	--add-include-path=$(G_IR_INCLUDEDIR)	\
	--include=Bunnyio-2.0 --include=BdkPixbuf-2.0 --include=Bango-1.0 --include=bairo-1.0	\
	--pkg-export=bdk-2.0	\
	--cflags-begin	\
	-DBDK_COMPILATION -I../.. -I../../bdk -I.../../bdk/win32	\
	--cflags-end	\
	--c-include=bdk/bdk.h	\
	--filelist=Bdk_2_0_gir_list	\
	-L.\vs$(VSVER)\$(CFG)\$(PLAT)\bin	\
	-o $@

vs$(VSVER)\$(CFG)\$(PLAT)\bin\Bdk-2.0.typelib: vs$(VSVER)\$(CFG)\$(PLAT)\bin\Bdk-2.0.gir
	@-echo Compiling $@...
	$(G_IR_COMPILER)	\
	--includedir=$(@D:\=/) --debug --verbose	\
	$(@R:\=/).gir	\
	-o $@

vs$(VSVER)\$(CFG)\$(PLAT)\bin\Btk-2.0.gir: Btk_2_0_gir_list 
	@-echo Generating $@...
	$(PYTHON) $(G_IR_SCANNER)	\
	--verbose -no-libtool	\
	--namespace=Btk	\
	--nsversion=2.0	\
		\
	--library=btk-win32-2.0 --library=bdk-win32-2.0	\
		\
	--add-include-path=./vs$(VSVER)/$(CFG)/$(PLAT)/bin	\
	--add-include-path=$(G_IR_INCLUDEDIR)	\
	--include=Batk-1.0	\
	--pkg-export=btk+-2.0	\
	--cflags-begin	\
	-DBTK_VERSION="2.24.33" -DBTK_BINARY_VERSION="2.10.0" -DBTK_COMPILATION -DBTK_DISABLE_DEPRECATED -DBTK_FILE_SYSTEM_ENABLE_UNSUPPORTED -DBTK_PRINT_BACKEND_ENABLE_UNSUPPORTED -DBTK_LIBDIR=\"/dummy/lib\" -DBTK_DATADIR=\"/dummy/share\" -DBTK_DATA_PREFIX=\"/dummy\" -DBTK_SYSCONFDIR=\"/dummy/etc\" -DBTK_HOST=\"$(AT_PLAT)-pc-vs$(VSVER)\" -DBTK_PRINT_BACKENDS=\"file\" -DINCLUDE_IM_am_et -DINCLUDE_IM_cedilla -DINCLUDE_IM_cyrillic_translit -DINCLUDE_IM_ime -DINCLUDE_IM_inuktitu -DINCLUDE_IM_ipa -DINCLUDE_IM_multipress -DINCLUDE_IM_thai -DINCLUDE_IM_ti_er -DINCLUDE_IM_ti_et -DINCLUDE_IM_viqr -UBDK_DISABLE_DEPRECATED -UBTK_DISABLE_DEPRECATED -DBTK_TEXT_USE_INTERNAL_UNSUPPORTED_API -I../.. -I../../btk -I../../bdk	\
	--cflags-end	\
	--warn-all --include-uninstalled=./vs$(VSVER)/$(CFG)/$(PLAT)/bin/Bdk-2.0.gir	\
	--filelist=Btk_2_0_gir_list	\
	-L.\vs$(VSVER)\$(CFG)\$(PLAT)\bin	\
	-o $@

vs$(VSVER)\$(CFG)\$(PLAT)\bin\Btk-2.0.typelib: vs$(VSVER)\$(CFG)\$(PLAT)\bin\Btk-2.0.gir
	@-echo Compiling $@...
	$(G_IR_COMPILER)	\
	--includedir=$(@D:\=/) --debug --verbose	\
	$(@R:\=/).gir	\
	-o $@

