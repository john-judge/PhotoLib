# Makefile for Independent JPEG Group's software

# This makefile is for Microsoft Visual C++ on Windows NT (and 95?).
# It builds the IJG library as a statically linkable library (.LIB),
# and builds the sample applications as console-mode apps.
# Thanks to Xingong Chang, Raymond Everly and others.

# Read installation instructions before saying "nmake" !!
# To build an optimized library without debug info, say "nmake nodebug=1".

# Pull in standard variable definitions
!include <win32.mak>

# You may want to adjust these compiler options:
CFLAGS= $(cflags) $(cdebug) $(cvars) -I.
# Generally, we recommend defining any configuration symbols in jconfig.h,
# NOT via -D switches here.

# Link-time options:
LDFLAGS= $(ldebug) $(conlflags)

# To link any special libraries, add the necessary commands here.
LDLIBS= $(conlibs)

# Put here the object file name for the correct system-dependent memory
# manager file.  For NT we suggest jmemnobs.obj, which expects the OS to
# provide adequate virtual memory.
SYSDEPMEM= $(INTDIR)\jmemnobs.obj

# miscellaneous OS-dependent stuff
# file deletion command
RM= del

# End of configurable options.

### EDT build env-specific

CFG=Release

!IFNDEF ARCH

!IFDEF PROGRAMW6432

ARCH=amd64

!else

ARCH=x86

!ENDIF

!ENDIF

PDVHOME = ..

!IFDEF BUILD_ID

BINDIR=$(PDVHOME)\bin\$(ARCH)
LIBDIR=$(PDVHOME)\lib\$(ARCH)

!ELSE

BINDIR=$(PDVHOME)
LIBDIR=$(PDVHOME)

!ENDIF

#call with ARCH set to x86 or amd64
CFGNAME=$(CFG)$(ARCH)
INTDIR=$(CFGNAME)

### End of EDT build env-specific

# source files: JPEG library proper
LIBSOURCES= jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
        jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
        jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c \
        jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c \
        jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c \
        jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c \
        jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
        jquant2.c jutils.c jmemmgr.c
# memmgr back ends: compile only one of these into a working library
SYSDEPSOURCES= jmemansi.c jmemname.c jmemnobs.c jmemdos.c jmemmac.c
# source files: cjpeg/djpeg/jpegtran applications, also rdjpgcom/wrjpgcom
APPSOURCES= cjpeg.c djpeg.c jpegtran.c rdjpgcom.c wrjpgcom.c cdjpeg.c \
        rdcolmap.c rdswitch.c transupp.c rdppm.c wrppm.c rdgif.c wrgif.c \
        rdtarga.c wrtarga.c rdbmp.c wrbmp.c rdrle.c wrrle.c
SOURCES= $(LIBSOURCES) $(SYSDEPSOURCES) $(APPSOURCES)
# files included by source files
INCLUDES= jchuff.h jdhuff.h jdct.h jerror.h jinclude.h jmemsys.h jmorecfg.h \
        jpegint.h jpeglib.h jversion.h cdjpeg.h cderror.h transupp.h
# documentation, test, and support files
DOCS= README install.doc usage.doc cjpeg.1 djpeg.1 jpegtran.1 rdjpgcom.1 \
        wrjpgcom.1 wizard.doc example.c libjpeg.doc structure.doc \
        coderules.doc filelist.doc change.log
MKFILES= configure makefile.cfg makefile.ansi makefile.unix makefile.bcc \
        makefile.mc6 makefile.dj makefile.wat makefile.vc makelib.ds \
        makeapps.ds makeproj.mac makcjpeg.st makdjpeg.st makljpeg.st \
        maktjpeg.st makefile.manx makefile.sas makefile.mms makefile.vms \
        makvms.opt
CONFIGFILES= jconfig.cfg jconfig.bcc jconfig.mc6 jconfig.dj jconfig.wat \
        jconfig.vc jconfig.mac jconfig.st jconfig.manx jconfig.sas \
        jconfig.vms
CONFIGUREFILES= config.guess config.sub install-sh ltconfig ltmain.sh
OTHERFILES= jconfig.doc ckconfig.c ansi2knr.c ansi2knr.1 jmemdosa.asm
TESTFILES= testorig.jpg testimg.ppm testimg.bmp testimg.jpg testprog.jpg \
        testimgp.jpg
DISTFILES= $(DOCS) $(MKFILES) $(CONFIGFILES) $(SOURCES) $(INCLUDES) \
        $(CONFIGUREFILES) $(OTHERFILES) $(TESTFILES)
# library object files common to compression and decompression
COMOBJECTS= $(INTDIR)\jcomapi.obj $(INTDIR)\jutils.obj $(INTDIR)\jerror.obj $(INTDIR)\jmemmgr.obj $(SYSDEPMEM)
# compression library object files
CLIBOBJECTS= $(INTDIR)\jcapimin.obj $(INTDIR)\jcapistd.obj $(INTDIR)\jctrans.obj $(INTDIR)\jcparam.obj $(INTDIR)\jdatadst.obj \
        $(INTDIR)\jcinit.obj $(INTDIR)\jcmaster.obj $(INTDIR)\jcmarker.obj $(INTDIR)\jcmainct.obj $(INTDIR)\jcprepct.obj \
        $(INTDIR)\jccoefct.obj $(INTDIR)\jccolor.obj $(INTDIR)\jcsample.obj $(INTDIR)\jchuff.obj $(INTDIR)\jcphuff.obj \
        $(INTDIR)\jcdctmgr.obj $(INTDIR)\jfdctfst.obj $(INTDIR)\jfdctflt.obj $(INTDIR)\jfdctint.obj
# decompression library object files
DLIBOBJECTS= $(INTDIR)\jdapimin.obj $(INTDIR)\jdapistd.obj $(INTDIR)\jdtrans.obj $(INTDIR)\jdatasrc.obj \
        $(INTDIR)\jdmaster.obj $(INTDIR)\jdinput.obj $(INTDIR)\jdmarker.obj $(INTDIR)\jdhuff.obj $(INTDIR)\jdphuff.obj \
        $(INTDIR)\jdmainct.obj $(INTDIR)\jdcoefct.obj $(INTDIR)\jdpostct.obj $(INTDIR)\jddctmgr.obj $(INTDIR)\jidctfst.obj \
        $(INTDIR)\jidctflt.obj $(INTDIR)\jidctint.obj $(INTDIR)\jidctred.obj $(INTDIR)\jdsample.obj $(INTDIR)\jdcolor.obj \
        $(INTDIR)\jquant1.obj $(INTDIR)\jquant2.obj $(INTDIR)\jdmerge.obj
# These objectfiles are included in $(LIBDIR)\libjpeg.lib
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)
# object files for sample applications (excluding library files)
COBJECTS= $(INTDIR)\cjpeg.obj $(INTDIR)\rdppm.obj $(INTDIR)\rdgif.obj $(INTDIR)\rdtarga.obj $(INTDIR)\rdrle.obj $(INTDIR)\rdbmp.obj \
        $(INTDIR)\rdswitch.obj $(INTDIR)\cdjpeg.obj
DOBJECTS= $(INTDIR)\djpeg.obj $(INTDIR)\wrppm.obj $(INTDIR)\wrgif.obj $(INTDIR)\wrtarga.obj $(INTDIR)\wrrle.obj $(INTDIR)\wrbmp.obj \
        $(INTDIR)\rdcolmap.obj $(INTDIR)\cdjpeg.obj
TROBJECTS= $(INTDIR)\jpegtran.obj $(INTDIR)\rdswitch.obj $(INTDIR)\cdjpeg.obj $(INTDIR)\transupp.obj

# Template command for compiling .c to .obj
{}.c{$(INTDIR)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(INTDIR)\ $<

# EDT: our stuff just needs the lib, but can use ALLTARGETS and get all the various .exes too.
TARGETS= $(LIBDIR)\libjpeg.lib 

ALLTARGETS= $(LIBDIR)\libjpeg.lib $(BINDIR)\cjpeg.exe $(BINDIR)\djpeg.exe $(BINDIR)\jpegtran.exe $(BINDIR)\rdjpgcom.exe $(BINDIR)\wrjpgcom.exe

all: verbose dir $(TARGETS) 

$(BINDIR)::
    @echo BINDIR $(BINDIR)
	@if not exist "$(BINDIR)/" mkdir $(BINDIR)

$(LIBDIR)::
    @echo LIBDIR $(LIBDIR)
	@if not exist "$(LIBDIR)/" mkdir $(LIBDIR)

$(INTDIR)::
    @echo INTDIR $(INTDIR)
	@if not exist "$(INTDIR)/" mkdir $(INTDIR)

dir: $(BINDIR) $(LIBDIR) $(INTDIR)

verbose:
    @echo TARGETS $(TARGETS)

$(LIBDIR)\libjpeg.lib: $(LIBOBJECTS)
	-$(RM) $(LIBDIR)\libjpeg.lib
	lib -out:$(LIBDIR)\libjpeg.lib  $(LIBOBJECTS)

$(BINDIR)\cjpeg.exe: $(COBJECTS) $(LIBDIR)\libjpeg.lib
	$(link) $(LDFLAGS) -out:$(BINDIR)\cjpeg.exe $(COBJECTS) $(LIBDIR)\libjpeg.lib $(LDLIBS)

$(BINDIR)\djpeg.exe: $(DOBJECTS) $(LIBDIR)\libjpeg.lib
	$(link) $(LDFLAGS) -out:$(BINDIR)\djpeg.exe $(DOBJECTS) $(LIBDIR)\libjpeg.lib $(LDLIBS)

$(BINDIR)\jpegtran.exe: $(TROBJECTS) $(LIBDIR)\libjpeg.lib
	$(link) $(LDFLAGS) -out:$(BINDIR)\jpegtran.exe $(TROBJECTS) $(LIBDIR)\libjpeg.lib $(LDLIBS)

$(BINDIR)\rdjpgcom.exe: $(INTDIR)\rdjpgcom.obj
	$(link) $(LDFLAGS) -out:$(BINDIR)\rdjpgcom.exe $(INTDIR)\rdjpgcom.obj $(LDLIBS)

$(BINDIR)\wrjpgcom.exe: $(INTDIR)\wrjpgcom.obj
	$(link) $(LDFLAGS) -out:$(BINDIR)\wrjpgcom.exe $(INTDIR)\wrjpgcom.obj $(LDLIBS)

clean:
	-$(RM) $(INTDIR)\*.obj $(TARGETS) 2>nul
	-$(RM) testout* 2>nul

test: $(BINDIR)\cjpeg.exe $(BINDIR)\djpeg.exe $(BINDIR)\jpegtran.exe
	$(RM) testout*
	.\djpeg -dct int -ppm -outfile testout.ppm  testorig.jpg
	.\djpeg -dct int -bmp -colors 256 -outfile testout.bmp  testorig.jpg
	.\cjpeg -dct int -outfile testout.jpg  testimg.ppm
	.\djpeg -dct int -ppm -outfile testoutp.ppm testprog.jpg
	.\cjpeg -dct int -progressive -opt -outfile testoutp.jpg testimg.ppm
	.\jpegtran -outfile testoutt.jpg testprog.jpg
	fc /b testimg.ppm testout.ppm
	fc /b testimg.bmp testout.bmp
	fc /b testimg.jpg testout.jpg
	fc /b testimg.ppm testoutp.ppm
	fc /b testimgp.jpg testoutp.jpg
	fc /b testorig.jpg testoutt.jpg


$(INTDIR)\jcapimin.obj: jcapimin.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcapistd.obj: jcapistd.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jccoefct.obj: jccoefct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jccolor.obj: jccolor.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcdctmgr.obj: jcdctmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jchuff.obj: jchuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jchuff.h
$(INTDIR)\jcinit.obj: jcinit.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcmainct.obj: jcmainct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcmarker.obj: jcmarker.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcmaster.obj: jcmaster.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcomapi.obj: jcomapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcparam.obj: jcparam.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcphuff.obj: jcphuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jchuff.h
$(INTDIR)\jcprepct.obj: jcprepct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jcsample.obj: jcsample.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jctrans.obj: jctrans.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdapimin.obj: jdapimin.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdapistd.obj: jdapistd.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdatadst.obj: jdatadst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h
$(INTDIR)\jdatasrc.obj: jdatasrc.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h
$(INTDIR)\jdcoefct.obj: jdcoefct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdcolor.obj: jdcolor.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jddctmgr.obj: jddctmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jdhuff.obj: jdhuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdhuff.h
$(INTDIR)\jdinput.obj: jdinput.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdmainct.obj: jdmainct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdmarker.obj: jdmarker.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdmaster.obj: jdmaster.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdmerge.obj: jdmerge.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdphuff.obj: jdphuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdhuff.h
$(INTDIR)\jdpostct.obj: jdpostct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdsample.obj: jdsample.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jdtrans.obj: jdtrans.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jerror.obj: jerror.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jversion.h jerror.h
$(INTDIR)\jfdctflt.obj: jfdctflt.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jfdctfst.obj: jfdctfst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jfdctint.obj: jfdctint.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jidctflt.obj: jidctflt.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jidctfst.obj: jidctfst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jidctint.obj: jidctint.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jidctred.obj: jidctred.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
$(INTDIR)\jquant1.obj: jquant1.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jquant2.obj: jquant2.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jutils.obj: jutils.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
$(INTDIR)\jmemmgr.obj: jmemmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
$(INTDIR)\jmemansi.obj: jmemansi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
$(INTDIR)\jmemname.obj: jmemname.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
$(INTDIR)\jmemnobs.obj: jmemnobs.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
$(INTDIR)\jmemdos.obj: jmemdos.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
$(INTDIR)\jmemmac.obj: jmemmac.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
$(INTDIR)\cjpeg.obj: cjpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h jversion.h
$(INTDIR)\djpeg.obj: djpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h jversion.h
$(INTDIR)\jpegtran.obj: jpegtran.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h transupp.h jversion.h
$(INTDIR)\rdjpgcom.obj: rdjpgcom.c jinclude.h jconfig.h
$(INTDIR)\wrjpgcom.obj: wrjpgcom.c jinclude.h jconfig.h
$(INTDIR)\cdjpeg.obj: cdjpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\rdcolmap.obj: rdcolmap.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\rdswitch.obj: rdswitch.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\transupp.obj: transupp.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h transupp.h
$(INTDIR)\rdppm.obj: rdppm.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\wrppm.obj: wrppm.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\rdgif.obj: rdgif.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\wrgif.obj: wrgif.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\rdtarga.obj: rdtarga.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\wrtarga.obj: wrtarga.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\rdbmp.obj: rdbmp.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\wrbmp.obj: wrbmp.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\rdrle.obj: rdrle.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
$(INTDIR)\wrrle.obj: wrrle.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
