# Makefile for the Linux version of libmsgbox.so
#
# NOTE: This needs to be run on a Linux system! 
#
NSOSTUB=./nsostub.sh

GCC=gcc
LD=ld
CFLAGS=-g

TMPFILE=tmp


all: libmsgbox.nso

libmsgbox.nso: libmsgbox.so
	$(NSOSTUB) $< > $(TMPFILE).c
	$(GCC) $(CFLAGS) -c $(TMPFILE).c
	$(LD) -shared -o $@ $(TMPFILE).o
	ls -l $@	
	

libmsgbox.so: libmsgbox.o
	$(LD) -shared -o $@ $< 

clean:
	rm -f *.o *.so *.nso *~ $(TMPFILE).c 
