# NMake Makefile to build Introspection Files for BTK+

!include detectenv-msvc.mak

APIVERSION = 2.0

CHECK_PACKAGE = bdk-pixbuf-2.0 batk bangobairo bunnyio-2.0

built_install_girs =	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Bdk-$(APIVERSION).gir	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Btk-$(APIVERSION).gir

built_install_typelibs =	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Bdk-$(APIVERSION).typelib	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Btk-$(APIVERSION).typelib

!include introspection-msvc.mak

!if "$(BUILD_INTROSPECTION)" == "TRUE"

!if "$(PLAT)" == "x64"
AT_PLAT=x86_64
!else
AT_PLAT=i686
!endif

all: setgirbuildenv $(built_install_girs) $(built_install_typelibs)

setgirbuildenv:
	@set PYTHONPATH=$(PREFIX)\lib\bobject-introspection
	@set PATH=vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(PREFIX)\bin;$(PATH)
	@set PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)
	@set LIB=vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(LIB)

!include introspection.body.mak

install-introspection: all 
	@-copy vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.gir $(G_IR_INCLUDEDIR)
	@-copy /b vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.typelib $(G_IR_TYPELIBDIR)

!else
all:
	@-echo $(ERROR_MSG)
!endif

clean:
	@-del /f/q vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.typelib
	@-del /f/q vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.gir
