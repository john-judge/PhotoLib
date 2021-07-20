
!include ../makefile.def

CC = cl

HEADER_DIR = ..
PDVHOME = ..

CFLAGS = /EHsc $(OPTIM) -MD -DWIN32 -nologo -I$(HEADER_DIR) -DUSE_MMX 
CCFLAGS = $(OPTIM) -MD -DWIN32 -nologo -I$(HEADER_DIR) -DUSE_MMX

LIBS = pdvlib.lib edt_mmx.lib

LINK32 = link
DLLFLAGS = /dll /nologo /link50compat /map

LIB32 = link -lib

LIBOBJECTS = \
	$(CFGNAME)\BufferTimerData.obj \
	$(CFGNAME)\ColorBalance.obj \
	$(CFGNAME)\EdtBGSubtractor.obj \
	$(CFGNAME)\EdtBufHeader.obj \
	$(CFGNAME)\EdtDataHeader.obj \
	$(CFGNAME)\EdtHistogram.obj \
	$(CFGNAME)\EdtImage.obj \
	$(CFGNAME)\EdtImageData.obj \
	$(CFGNAME)\EdtImgArith.obj \
	$(CFGNAME)\EdtImgDyadOp.obj \
	$(CFGNAME)\EdtImgMinMax.obj \
	$(CFGNAME)\EdtImgOp.obj \
	$(CFGNAME)\EdtImgProfile.obj \
	$(CFGNAME)\EdtLut.obj \
	$(CFGNAME)\EdtScaledLut.obj 

TARGET=$(LIBDIR)\edtimage.lib
$(TARGET): dir $(LIBOBJECTS) $(INCLUDES) 
	 link /lib /out:$(TARGET) $(LIBOBJECTS)


dir:
	@if not exist "$(CFGNAME)/" mkdir $(CFGNAME)

clean: objclean

objclean:
	-del $(TARGET) 2>nul
    -if exist $(CFGNAME) del /Q $(LIBOBJECTS) 2>nul


#{}.c{$(CFGNAME)}.obj:
#	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)/ $<

{}.cpp{$(CFGNAME)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)/ $<









