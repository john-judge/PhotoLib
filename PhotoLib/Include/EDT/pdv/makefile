#makefile for pdv executables
# Copyright EDT 1998

!include makefile.def

!include includes.mk 

PDVHOME=$(MAKEDIR)

# _CRT_SECURE_NO_DEPRECATE gets rid of warnings about strcpy 
# being unsafe (we don't want to use proprietary M$ strcpy_s)
# It is useful if using Vis. Studio 2005 or its 'cl' compiler

TARGETLIB=pdvlib.lib
TARGETDLL=pdvlib.dll

ARCHLIB=$(LIBDIR)\$(TARGETLIB)
ARCHDLL=$(BINDIR)\$(TARGETDLL)

EXTRA_TARGETS = pciload.exe pciediag.exe

all: $(LIBS) $(TARGETS) $(CPP_TARGETS) $(EXTRA_TARGETS)

$(TARGETS) $(CPP_TARGETS): $(CFGNAME) $(CFGNAME)\$*.obj $(INCLUDES) $(LIBS)
	$(CC) $(CFLAGS) $(CFGNAME)\$*.obj /Fe$(BINDIR)\$*.exe $(LIBS) $(WINLIBS) /link $(LDFLAGS)
	@if exist "$(BINDIR)\$@.manifest" mt.exe /nologo -manifest $(BINDIR)\$*.exe.manifest -outputresource:$(BINDIR)\$*.exe
	@if exist "$(BINDIR)\$@.manifest" del $(BINDIR)\$*.exe.manifest


#static linking

$(ARCHLIB) pdvlib: dir $(LIBOBJS) $(INCLUDES) 
	$(LINK32) $(DLLFLAGS) /out:$(ARCHDLL) /implib:$(ARCHLIB) $(LIBOBJS) $(MMX_LIB)
	@if exist "$(ARCHDLL).manifest" mt.exe /nologo -manifest $(ARCHDLL).manifest -outputresource:$(ARCHDLL);2
	@if exist "$(ARCHDLL).manifest" del $(ARCHDLL).manifest

dir: $(CFGNAME) $(BINDIR) $(LIBDIR)

$(CFGNAME)::
	@echo CFGNAME $(CFGNAME)
	@if not exist "$(CFGNAME)\" mkdir $(CFGNAME)

$(BINDIR)::
	@echo BINDIR $(BINDIR)
	@if not exist "$(BINDIR)\" mkdir $(BINDIR)

$(LIBDIR)::
	@echo LIBDIR $(LIBDIR)
	@if not exist "$(LIBDIR)\" mkdir $(LIBDIR)

$(LIBDIR)\libtiff.lib libtiff:
	cd $(TIFFDIR)
	$(MAKE) -f makefile.edtwin
	cd $(MAKEDIR)

$(LIBDIR)\libjpeg.lib libjpeg:
	cd $(JPEGDIR)
	del /Q *.obj 2>NUL
	$(MAKE) -f makefile
	cd $(MAKEDIR)
	
PCILOADOBJS = $(CFGNAME)\pciload_main.obj $(CFGNAME)\pciload_fn.obj \
	$(CFGNAME)\pciload_4013e.obj $(CFGNAME)\pciload_4013xla.obj $(CFGNAME)\pciload_4028xla.obj $(CFGNAME)\pciload_spi.obj

pciload.exe : $(PCILOADOBJS) $(INCLUDES) 
	$(LINK32) /out:$(BINDIR)\$@ $(PCILOADOBJS) $(LIBS)
	if exist "$(BINDIR)\$@.manifest" mt.exe -manifest $(BINDIR)\$*.exe.manifest -outputresource:$(BINDIR)\$*.exe
	@if exist "$(BINDIR)\$@.manifest" del $(BINDIR)\$*.exe.manifest
	@if exist "$(BINDIR)\$*.lib" del $(BINDIR)\$*.lib

pciediag.exe pciediag:
	cd pciediag_src
	$(MAKE) -C -f makefile
	cd $(MAKEDIR)

$(LIBDIR)\libjpeg.lib libjpeg: FRC
	cd $(JPEGDIR)
	$(MAKE) -f makefile nodebug=1
	cd $(MAKEDIR)


$(LIBDIR)\pdvplus.lib pdvplus: FRC
	cd pdvplus
	$(MAKE) -C -f makefile pdvplus
	cd $(MAKEDIR)

$(LIBDIR)\dispatch.lib dispatch: FRC
	cd dispatch
	$(MAKE) -C -f makefile
	cd $(MAKEDIR)

$(LIBDIR)\edtimage.lib edtimage: FRC
	cd edtimage
	$(MAKE) -C -f makefile
	cd $(MAKEDIR)

$(LIBDIR)\imgfiles.lib imgfiles: FRC
	cd imgfiles
	$(MAKE) -C -f makefile
	cd $(MAKEDIR)

$(LIBDIR)\clseredt.lib clseredt: FRC
	cd edt_camlink
	$(MAKE) -C -f makefile
	cd $(MAKEDIR)

objclean: 
	-del *.exe.manifest $(CFGNAME)\*.obj *.ilk *.pch 

clean: objclean
	-del /Q $(TARGETS) $(EXTRA_TARGETS) $(CPP_TARGETS) $(TARGETLIB) $(ARCHLIB) $(TARGETDLL) $(ARCHDLL) 2>NUL
	cd $(MAKEDIR)
	cd $(JPEGDIR)
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)
	cd pdvplus
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)
	cd dispatch
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)
	cd imgfiles
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)
	cd edtimage
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)
	cd edt_camlink
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)
	cd pciediag_src
	-$(MAKE) -C -f makefile clean
	cd $(MAKEDIR)

{}.c{$(CFGNAME)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)\ $<

{}.cpp{$(CFGNAME)}.obj:
	$(CC) -c $(CCFLAGS) -Fo$(CFGNAME)\ $<


FRC:
	
