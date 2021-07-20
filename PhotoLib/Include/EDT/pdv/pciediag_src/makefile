
!include ../makefile.def

CC = cl

LIBSRC = ..
HEADER_DIR = ..
PDVHOME = ..
PCDHOME = ..

# override makefile.def BINDIR if installed not build
!IFNDEF BUILD_ID
BINDIR = ..
LIBDIR = ..
!ENDIF

TARGET=$(BINDIR)\pciediag.exe

INCLUDES = $(INCLUDES) pciecfg.h

CFLAGS = $(OPTIM) -DWIN32 -nologo -I$(HEADER_DIR) 
CCFLAGS = $(OPTIM) -DWIN32 -nologo -I$(HEADER_DIR) 

LINK32 = link

LIB32 = link -lib

LIBOBJS = \
     $(CFGNAME)\libedt.obj \
     $(CFGNAME)\libedt_timing.obj \
     $(CFGNAME)\edt_error.obj \
     $(CFGNAME)\edt_os_nt.obj \
     $(CFGNAME)\edt_flash.obj

!IF "$(EDTDEV)" == "pcd"
LIBOBJS = $(LIBOBJS) \
    $(CFGNAME)\edt_ss_vco.obj \
    $(CFGNAME)\edt_vco.obj
!ENDIF

OBJS = \
	$(CFGNAME)\pciediag.obj $(CFGNAME)\pciecfg.obj

all: dir $(TARGET)

$(TARGET): $(LIBOBJS) $(INCLUDES) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBOBJS) /Fe$*.exe /link $(LDFLAGS)
	@if exist "$*.exe.manifest" mt.exe /nologo -manifest $*.exe.manifest -outputresource:$*.exe
    @if exist "$*.exe.manifest" del $*.exe.manifest
    @if exist "$*.exp" del $*.exp
    @if exist "$*.lib" del $*.lib

clean:
	-del /Q $(TARGET) $(OBJS) 2>nul
    -if exist $(CFGNAME) del $(LIBOBJS) 2>nul

dir:
	if not exist $(CFGNAME)/ mkdir $(CFGNAME)

{$(LIBSRC)}.c{$(CFGNAME)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)\ $<

{}.cpp{$(CFGNAME)}.obj:
	$(CC) -c $(CCFLAGS) -Fo$(CFGNAME)\ $<









