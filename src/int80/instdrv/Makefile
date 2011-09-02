# $Id: Makefile,v 1.1 2001/04/25 18:33:24 mvines Exp $

instdrv.exe: instdrv.c
	$(CC) -mno-cygwin -o $@ $^
	strip $@

clean:
	rm -f instdrv.exe
