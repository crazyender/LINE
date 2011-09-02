/**
 * The Linux version of libmsgbox.so
 * $Id: libmsgbox.c,v 1.1.1.1 2001/03/07 18:34:17 mvines Exp $
 */ 
#include <stdio.h>

void msgbox(char *msg)
{
  printf("msgbox(%s)\n", msg);
}

void msgbox2(char *title, char *msg)
{
  printf("msgbox(title='%s', msg='%s')\n", title, msg);
}

