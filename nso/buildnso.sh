#!/bin/bash
# $Id: buildnso.sh,v 1.1.1.1 2001/03/07 18:34:07 mvines Exp $
#
# Sample script of how to build a nso stub shared object 

if [ -z "$1" ]; then  
  echo usage: buildnso.sh libwhatever.so
  exit;
fi

BASENAME=`basename $1 .so`

SRC=$BASENAME.c
OBJ=$BASENAME.o
LIB=$BASENAME.nso

./nsostub.sh $1 > $SRC
gcc -c $SRC -o $OBJ
ld -shared -o $LIB $OBJ

ls -l $LIB
