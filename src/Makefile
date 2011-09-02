# Topmost Makefile.  Just do a 'make' to build everything.  
# The binaries will end up in the parent directory
# $Id: Makefile,v 1.4 2001/03/23 17:42:46 mvines Exp $
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

all: newversion
	$(MAKE) -C Line 
	$(MAKE) -C Linexec
	$(MAKE) -C linelog

.PHONY : newversion

newversion:
	rm -f $(COMMON)/genver.h


strip: all
	strip ../Line.exe
	strip ../Linexec.exe
	strip ../linelog.exe
	strip ../logconf.exe


include Rules.mk


