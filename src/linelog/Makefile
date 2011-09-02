# LINELog Makefile
# $Id: Makefile,v 1.3 2001/03/23 17:42:46 mvines Exp $
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
LINELOG=../../Linelog.exe
LOGCONF=../../logconf.exe
LOGCON=../../logcon.dll
EXE= $(LINELOG) $(LOGCONF) $(LOGCON)

SRC=linelog.c logconf.c 
DEP=$(SRC:.c=.d)

include ../Rules.mk

LDFLAGS=-luser32 -g
CFLAGS=-g -Wall -I../common -DLOG_SOURCE=LOG_LINELOG

$(LINELOG): linelog.o 
	$(CC) -o $@ $(LDFLAGS) -mwindows -e _mainCRTStartup $^

$(LOGCONF): logconf.o
	$(CC) -o $@ $(LDFLAGS) $^
	
$(LOGCON):
	$(MAKE) -C logcon
