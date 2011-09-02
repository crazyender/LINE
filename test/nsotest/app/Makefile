# Makefile for the NSO test program
#
# NOTE: this should be run on a Linux system
#
GCC=gcc
LD=ld
CFLAGS=-g

nsotest: nsotest.o ../linux/libmsgbox.so
	$(GCC) -o $@ $< -L../linux/ -lmsgbox

clean:
	rm -f nsotest *.o *~ 
