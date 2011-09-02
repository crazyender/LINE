# Assumes that logconf.exe has been built and is located in ../../..
# $Id: Makefile,v 1.4 2001/03/23 17:42:46 mvines Exp $

EXE=all
SRC=logcon.c
OBJ=$(SRC:.c=.o) 
DEP=$(SRC:.c=.d)

CUSTOMCLEAN=1

include ../../Rules.mk

DLLTOOL=dlltool
LD=ld

NAME=logcon
ENTRY=_DllMain@12

CFLAGS += -DLOG_SOURCE=LOG_LINELOG

DLLNAME=$(NAME).dll
DEFFILE=../handlerdll.def
LIBS=-lcygwin -luser32 -lkernel32 -ladvapi32

COPYDIR=../../..

LIBFILE=$(NAME).a
BASEFILE=$(NAME).base
EXPFILE=$(NAME).exp

all: $(LIBFILE)
	$(COPYDIR)/logconf -r $(COPYDIR)/$(DLLNAME)
#	$(COPYDIR)/logconf -c $(DLLNAME) -l 1


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



customclean:
	rm -f $(COPYDIR)/$(DLLNAME) $(LIBFILE) $(BASEFILE) $(EXPFILE) 

