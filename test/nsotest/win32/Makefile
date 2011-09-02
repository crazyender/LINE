# Makefile for the Win32 version of libmsgbox.so
# $Id: Makefile,v 1.1.1.1 2001/03/07 18:34:18 mvines Exp $
#
DLLTOOL=dlltool
LD=ld

NAME=libmsgbox
ENTRY=__cygwin_dll_entry@12

CFLAGS=-Wall

DLLNAME=$(NAME).so.dll
DEFFILE=$(NAME).def
LIBS=-lcygwin -lkernel32 -luser32

COPYDIR=../../../nso/

SRC=libmsgbox.c
OBJ=$(SRC:.c=.o) 
DEP=$(SRC:.c=.d)


LIBFILE=$(NAME).a
BASEFILE=$(NAME).base
EXPFILE=$(NAME).exp

all: $(LIBFILE)


# builds the import library
$(LIBFILE): $(DLLNAME) $(DEFFILE) 
	$(DLLTOOL) --def $(DEFFILE) --dllname $(DLLNAME) --output-lib $(LIBFILE)

$(DLLNAME): $(OBJ) $(DEF)
	$(LD) -s --base-file $(BASEFILE) -dll -o $(DLLNAME) $(OBJ) $(LIBS) -e $(ENTRY)
	$(DLLTOOL) --dllname $(EXE) --def $(DEFFILE) --base-file=$(BASEFILE) --output-exp $(EXPFILE)
	$(LD) -s --base-file $(BASEFILE) $(EXPFILE) -dll -o $(DLLNAME) $(OBJ) $(LIBS) -e $(ENTRY)
	$(DLLTOOL) --dllname $(DLLNAME) --def $(DEFFILE) --base-file $(BASEFILE) --output-exp $(EXPFILE)
	$(LD) $(EXPFILE) --dll -o $(DLLNAME) $(OBJ) $(LIBS) -e $(ENTRY)
	cp $(DLLNAME) $(COPYDIR)


clean:
	rm -f $(DLLNAME) $(COPYDIR)/$(DLLNAME) $(OBJ) $(LIBFILE) $(BASEFILE) $(EXPFILE) $(DEP) *~

