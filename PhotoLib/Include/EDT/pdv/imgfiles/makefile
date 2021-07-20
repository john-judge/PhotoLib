!include ../makefile.def

CFG=Release
PDVHOME=..
INTDIR=.\$(CFGNAME)

CFLAGS = $(OPTIM) -D_MBCS -DWIN32 -nologo $(CFLAGS) -I$(PDVHOME)\pdvplus -I$(PDVHOME)\tiff\libtiff -I$(PDVHOME)\jpeg-6b

LIBOBJS = \
$(INTDIR)\imagefile.obj \
$(INTDIR)\imagefilemgr.obj \
$(INTDIR)\imagefileraw.obj \
$(INTDIR)\imagefilebmp.obj \
$(INTDIR)\Imagefiletiff.obj \
$(INTDIR)\imagefilejpeg.obj \
$(INTDIR)\imagefiletype.obj 

TARGETLIB = $(LIBDIR)\imgfiles.lib

all : $(TARGETLIB)

dir : $(LIBDIR) $(INTDIR)

$(LIBDIR):
	if not exist $(LIBDIR)/ mkdir $(LIBDIR)

$(INTDIR):
	if not exist $(INTDIR)/ mkdir $(INTDIR)

$(TARGETLIB) : dir $(LIBOBJS)
	$(LINK32) /lib /OUT:$(TARGETLIB) $(LIBOBJS) 

{}.c{$(INTDIR)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(INTDIR)\ $<

{}.cpp{$(INTDIR)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(INTDIR)\ $<

clean: 
    del /Q $(TARGETLIB) 2>nul
    -if exist $(INTDIR) del /Q $(LIBOBJS) 2>nul
