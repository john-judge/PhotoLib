
!include ../makefile.def

CC = cl

HEADER_DIR = ..
PDVHOME= ..

CFLAGS = $(OPTIM) -EHsc -DWIN32 -nologo -I$(HEADER_DIR) 
CCFLAGS = $(OPTIM) -EHsc -DWIN32 -nologo -I$(HEADER_DIR) 

LIBS = pdvlib.lib edt_mmx.lib

LINK32 = link
DLLFLAGS = /dll /nologo /link50compat /map

LIB32 = link -lib

LIBOBJECTS = \
	$(CFGNAME)\PdvInput.obj \
	$(CFGNAME)\PdvConfig.obj \
	$(CFGNAME)\PdvConfigSet.obj \
	$(CFGNAME)\PdvDevice.obj \
	$(CFGNAME)\PdvDeviceSet.obj


PDVPLUSLIB=$(LIBDIR)\pdvplus.lib

$(PDVPLUSLIB) pdvplus: dir $(LIBOBJECTS) $(INCLUDES) 
	 link /lib /out:$(PDVPLUSLIB) $(LIBOBJECTS)

dir:
	@if not exist "$(CFGNAME)/" mkdir $(CFGNAME)

clean: objclean

objclean:
	-del $(LIBOBJECTS) $(PDVPLUSLIB) 2>nul


#{}.c{$(CFGNAME)}.obj:
#	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)/ $<

{}.cpp{$(CFGNAME)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)/ $<









