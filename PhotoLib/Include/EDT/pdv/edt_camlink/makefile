
!include ../makefile.def

CC = cl

LIBSRC=..
HEADER_DIR=..
PDVHOME=..

CLSEREDTLIB=$(LIBDIR)\clseredt.lib
CLSEREDTDLL=$(BINDIR)\clseredt.dll
CLSEREDTMANIFEST=$(CLSEREDTDLL).manifest
CLSEREDTMAP=$(CLSEREDTDLL).map
CLSEREDTEXP=$(CLSEREDTDLL).exp

INCLUDES = $(INCLUDES) edt_camlink.h

CFLAGS = $(OPTIM) -DWIN32 -nologo -I$(HEADER_DIR) 
CCFLAGS = $(OPTIM) -DWIN32 -nologo -D "EDT_CAMLINK_EXPORTS" -D "_USRDLL" -I$(HEADER_DIR) 

LINK32 = link
DLLFLAGS = /dll /nologo /link50compat /map $(LDFLAGS)

LIB32 = link -lib

OBJS=$(CFGNAME)\edt_camlink.obj

LIBOBJS = \
     $(CFGNAME)\libpdv.obj \
     $(CFGNAME)\libedt.obj \
     $(CFGNAME)\pdv_interlace.obj \
     $(CFGNAME)\pdv_irig.obj \
     $(CFGNAME)\pdv_dmy_image.obj \
     $(CFGNAME)\pdv_bayer_filter.obj \
     $(CFGNAME)\libedt_timing.obj \
     $(CFGNAME)\libdvu.obj \
     $(CFGNAME)\pciload_fn.obj \
     $(CFGNAME)\edt_error.obj \
     $(CFGNAME)\edt_os_nt.obj \
     $(CFGNAME)\edt_flash.obj

all: dir $(CLSEREDTLIB)

$(CLSEREDTLIB): $(LIBOBJS) $(DLLOBJS) $(INCLUDES) $(OBJS)
	$(LINK32) $(OBJS) $(LIBOBJS) $(DLLFLAGS) /out:$(CLSEREDTDLL) /implib:$(CLSEREDTLIB)
	@if exist $(CLSEREDTMANIFEST) mt.exe /nologo -manifest $(CLSEREDTMANIFEST) -outputresource:$(CLSEREDTDLL);2
	@if exist $(CLSEREDTMANIFEST) del $(CLSEREDTMANIFEST)
	@if exist $(CLSEREDTMAP) del $(CLSEREDTMAP)
	@if exist $(CLSEREDTEXP) del $(CLSEREDTEXP)

clean:
	-del /Q $(CLSEREDTDLL) $(CLSEREDTLIB) $(DLLOBJS) $(CFGNAME)\*.obj 2>nul

dir:
	if not exist $(CFGNAME)/ mkdir $(CFGNAME)
	if not exist $(LIBDIR)/ mkdir $(LIBDIR)
	if not exist $(BINDIR)/ mkdir $(BINDIR)

{$(LIBSRC)}.c{$(CFGNAME)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)\ $<

{}.cpp{$(CFGNAME)}.obj:
	$(CC) -c $(CCFLAGS) -Fo$(CFGNAME)\ $<


