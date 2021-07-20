!include ../makefile.def

PDVHOME=..

CFLAGS = $(CFLAGS) $(OPTIM)
CCFLAGS = $(CCFLAGS) $(OPTIM)

!IFNDEF BUILD_ID
LIBDIR=..
BINDIR=..
!ENDIF

INCLUDES = $(INCLUDES) \
    EdtDataValue.h \
    ErrorHandler.h \
    ParmObject.h \
    SocketConnection.h \
    StringUtils.h \
    EdtCmdPort.h \
    EdtLogStreambuf.h

LIBOBJS = \
    $(CFGNAME)/EdtDataValue.obj \
    $(CFGNAME)/ErrorHandler.obj \
    $(CFGNAME)/ParmObject.obj \
    $(CFGNAME)/SocketConnection.obj \
    $(CFGNAME)/StringUtils.obj 

all: test dir $(DISPATCHLIB) 

test:
	echo "cflags=$(CFLAGS) optim=$(OPTIM) "
	echo "BINDIR=$(BINDIR) LIBDIR=$(LIBDIR) "

DISPATCHLIB=dispatch.lib

all: $(DISPATCHLIB) 

$(DISPATCHLIB): dir $(LIBOBJS) $(INCLUDES) 
	lib /out:$(LIBDIR)\$*.lib $(LIBOBJS)
	@if exist "$(BINDIR)\$*.map" del $(BINDIR)\$*.map
	@if exist "$(BINDIR)\$*.exp" del $(BINDIR)\$*.exp

clean:
	-del /Q $(DISPATCHLIB) $(CFGNAME)\*.obj 2>NUL

dir:
	@if not exist "$(CFGNAME)/" mkdir $(CFGNAME)
	@if not exist "$(LIBDIR)/" mkdir $(LIBDIR)
	@if not exist "$(BINDIR)/" mkdir $(BINDIR)

{}.c{$(CFGNAME)}.obj:
	$(CC) -c $(CFLAGS) -Fo$(CFGNAME)\ $<

{}.cpp{$(CFGNAME)}.obj:
	$(CC) -c $(CCFLAGS) -Fo$(CFGNAME)\ $<


