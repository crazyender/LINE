# Makefile for Linexec.exe
# $Id: Makefile,v 1.5 2001/03/26 16:18:54 mvines Exp $
#
# Copyright (C) 2001  Michael Vines
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
EXE=all

LINEXEC=../../Linexec.exe
SRC=$(shell ls *.c) 
ASM_SRC=$(shell ls *.s)
OBJ=$(SRC:.c=.o) $(ASM_SRC:.s=.o) 
DEP=$(SRC:.c=.d)

LIBBINFMT=binfmt/binfmt.o

include ../Rules.mk

LDFLAGS += -luser32 -g -Wl,--image-base=0x65000000 
CFLAGS += -DLOG_SOURCE=LOG_LINEXEC

all : libcommon_check libbinfmt_check $(LINEXEC)

$(LINEXEC): $(OBJ) $(LIBCOMMON) $(LIBBINFMT)
	$(CC) -o $@ $(LDFLAGS) $^

.PHONY : libbinfmt_check

$(LIBBINFMT) libbinfmt_check:
	$(MAKE) -C binfmt

