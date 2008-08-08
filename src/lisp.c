/* Lisp main routine

   Copyright (c) 2008 Free Software Foundation, Inc.
   Copyright (c) 2001 Scott "Jerry" Lawrence.
   Copyright (c) 2005 Reuben Thomas.

   This file is part of GNU Zile.

   GNU Zile is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Zile is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Zile; see the file COPYING.  If not, write to the
   Free Software Foundation, Fifth Floor, 51 Franklin Street, Boston,
   MA 02111-1301, USA.  */

#include <stdio.h>
#include <assert.h>
#include "zile.h"
#include "extern.h"
#include "eval.h"


void
lisp_init (void)
{
  leNIL = leNew ("NIL");
  leT = leNew ("T");
}


void
lisp_finalise (void)
{
  leReallyWipe (leNIL);
  leReallyWipe (leT);
}


le *
lisp_read (getcCallback getcp, ungetcCallback ungetcp)
{
  int lineno = 0;
  struct le *list = NULL;

  list = parseInFile (getcp, ungetcp, list, &lineno);

  return list;
}


static FILE *fp = NULL;

static int
getc_file (void)
{
  return getc (fp);
}

static void
ungetc_file (int c)
{
  ungetc (c, fp);
}

le *
lisp_read_file (const char *file)
{
  le *list;
  fp = fopen (file, "r");

  if (fp == NULL)
    return NULL;

  list = lisp_read (getc_file, ungetc_file);
  fclose (fp);

  return list;
}
