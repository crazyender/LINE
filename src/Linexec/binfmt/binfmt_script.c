/*
 *  linux/fs/binfmt_script.c
 *
 *  Copyright (C) 1996  Martin von Löwis
 *  original #!-checking implemented by tytso.
 */

/** 
 * Modified heavily for LINE (from linux 2.2.5)
 * $Id: binfmt_script.c,v 1.1.1.1 2001/03/07 18:34:14 mvines Exp $
 */
 
//#define __VERBOSE__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "binfmts.h"
#include "../errno.h"
#include "../cygwin_errno.h"


static int load_script(struct linux_binprm *bprm);


struct linux_binfmt script_format = {
	NULL, load_script, NULL
};


static int load_script(struct linux_binprm *bprm)
{
	char *cp, *i_name, *i_name_start, *i_arg;
  int fd;
	char interp[128];
	int retval;

	if ((bprm->buf[0] != '#') || (bprm->buf[1] != '!') || (bprm->sh_bang)) 
		return -ENOEXEC;
	/*
	 * This section does the #! interpretation.
	 * Sorta complicated, but hopefully it will work.  -TYT
	 */

	bprm->sh_bang++;
  close(bprm->fd);

	bprm->buf[127] = '\0';
	if ((cp = strchr(bprm->buf, '\n')) == NULL)
		cp = bprm->buf+127;
	*cp = '\0';
	while (cp > bprm->buf) {
		cp--;
		if ((*cp == ' ') || (*cp == '\t'))
			*cp = '\0';
		else
			break;
	}
	for (cp = bprm->buf+2; (*cp == ' ') || (*cp == '\t'); cp++);
	if (*cp == '\0') 
		return -ENOEXEC; /* No interpreter name found */
	i_name_start = i_name = cp;
	i_arg = 0;
	for ( ; *cp && (*cp != ' ') && (*cp != '\t'); cp++) {
 		if (*cp == '/')
			i_name = cp+1;
	}
	while ((*cp == ' ') || (*cp == '\t'))
		*cp++ = '\0';
	if (*cp)
		i_arg = cp;
	strcpy (interp, i_name_start);
	/*
	 * OK, we've parsed out the interpreter name and
	 * (optional) argument.
	 * Splice in (1) the interpreter's name for argv[0]
	 *           (2) (optional) argument to interpreter
	 *           (3) filename of shell script (replace argv[0])
	 *
	 * This is done in reverse order, because of how the
	 * user environment and arguments are stored.
	 */
	remove_arg_zero(bprm);
	bprm->p = copy_strings(1, &bprm->filename, bprm->page, bprm->p);
	bprm->argc++;
	if (i_arg) {
		bprm->p = copy_strings(1, &i_arg, bprm->page, bprm->p);
		bprm->argc++;
	}
	bprm->p = copy_strings(1, &i_name, bprm->page, bprm->p);
	bprm->argc++;
	if (!bprm->p) 
		return -E2BIG;

	/*
	 * OK, now restart the process with the interpreter's dentry.
	 */
#ifdef __VERBOSE__
  printf("load_script(): using interpreter %s\n", interp);
#endif
  
  fd = open(interp, O_RDONLY); 
  if (fd < 0) return -errno;

	bprm->fd = fd;

  bzero(bprm->buf, sizeof(bprm->buf));  
  retval = read(fd, bprm->buf, sizeof(bprm->buf));
	if (retval < 0)
		return retval;

	return search_binary_handler(bprm);
}



int init_script_binfmt(void)
{
	return register_binfmt(&script_format);
}
