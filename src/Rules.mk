# Common stuff used by all the Makefiles
# $Id: Rules.mk,v 1.2 2001/03/26 16:18:54 mvines Exp $
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

ifeq ($(clean),1)
__all: clean
else
ifneq ($(EXE), '')
__all: $(EXE)
endif
endif


# location of the common/ subdirectory relative to the current directory 
COMMON=$(shell if [ -d common/ ]; then \
                  echo "common";  \
               else \
                  if [ -d ../common/ ]; then \
			echo "../common"; \
		  else \
			echo "../../common"; \
		  fi; \
	       fi)
	
LIBCOMMON=$(COMMON)/common.o

CFLAGS= -Wall -g -I$(COMMON)
LDFLAGS= -g


# Uncomment this when doing an actual LINE release
CFLAGS += -DLINE_VERSION=\"0.5\"


.PHONY : libcommon_check

libcommon_check: 
	$(MAKE) -C $(COMMON)

$(LIBCOMMON):
	$(MAKE) -C $(COMMON)


# Don't try to build dependencies if we are cleaning
ifneq ($(clean), 1)

%.d: %.c
	@echo "> Generating dependency information for $< <"
	@-$(SHELL) -ec '$(CC) $(CFLAGS) -M -MG $< | sed "s#$*\\.o[ :]*#& $@ #g" > $@'

# Make sure there are dependencies to include
ifneq ($(DEP), '')
-include $(DEP)	
endif

endif


clean: 
	rm -f $(EXE) $(OBJ) $(DEP) *~ *.bak
ifeq ($(CUSTOMCLEAN),1) 
	$(MAKE) clean=1 customclean
endif	
	@( \
	for D in `find . -maxdepth 1 -type d`; do \
		if [ -f $$D/Makefile -a ! $$D = "." ]; then \
		 	$(MAKE) -C $$D clean=1; \
	 	fi \
	done \
	)

