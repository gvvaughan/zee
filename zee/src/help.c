/* Self documentation facility functions
   Copyright (c) 1997-2004 Sandro Sigala.
   Copyright (c) 2003-2005 Reuben Thomas.
   All rights reserved.

   This file is part of Zee.

   Zee is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Zee is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with Zee; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/*	$Id$	*/

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zee.h"
#include "extern.h"
#include "paths.h"

DEFUN_INT("zee-version", zee_version)
  /*+
    Show the zee version.
    +*/
{
  minibuf_write("Zee " VERSION " of " CONFIGURE_DATE " on " CONFIGURE_HOST);

  return TRUE;
}
END_DEFUN

/*
 * Fetch the documentation of a function or variable from the
 * AUTODOC automatically generated file.
 */
static astr get_funcvar_doc(char *name, astr defval, int isfunc)
{
  FILE *f;
  astr buf, match, doc;
  int reading_doc = 0;

  if ((f = fopen(PATH_DATA "/AUTODOC", "r")) == NULL) {
    minibuf_error("Unable to read file `%s'",
                  PATH_DATA "/AUTODOC");
    return NULL;
  }

  match = astr_new();
  if (isfunc)
    astr_afmt(match, "\fF_%s", name);
  else
    astr_afmt(match, "\fV_%s", name);

  doc = astr_new();
  while ((buf = astr_fgets(f)) != NULL) {
    if (reading_doc) {
      if (*astr_char(buf, 0) == '\f') {
        astr_delete(buf);
        break;
      }
      if (isfunc || astr_len(defval) > 0) {
        astr_cat(doc, buf);
        astr_cat_cstr(doc, "\n");
      } else
        astr_cpy(defval, buf);
    } else if (!astr_cmp(buf, match))
      reading_doc = 1;
    astr_delete(buf);
  }

  fclose(f);
  astr_delete(match);

  if (!reading_doc) {
    minibuf_error("Cannot find documentation for `%s'", name);
    astr_delete(doc);
    return NULL;
  }

  return doc;
}

static void write_function_description(va_list ap)
{
  const char *name = va_arg(ap, const char *);
  astr doc = va_arg(ap, astr);

  bprintf("Function: %s\n\n"
          "Documentation:\n%s",
          name, astr_cstr(doc));
}

DEFUN_INT("describe-function", describe_function)
  /*+
    Display the full documentation of a function.
    +*/
{
  char *name;
  astr bufname, doc;

  name = minibuf_read_function_name("Describe function: ");
  if (name == NULL)
    return FALSE;

  if ((doc = get_funcvar_doc(name, NULL, TRUE)) == NULL)
    return FALSE;

  bufname = astr_new();
  astr_afmt(bufname, "*Help: function `%s'*", name);
  write_temp_buffer(astr_cstr(bufname), write_function_description,
                    name, doc);
  free(name);
  astr_delete(bufname);
  astr_delete(doc);

  return TRUE;
}
END_DEFUN

static void write_variable_description(va_list ap)
{
  char *name = va_arg(ap, char *);
  astr defval = va_arg(ap, astr);
  char *curval = va_arg(ap, char *);
  astr doc = va_arg(ap, astr);
  bprintf("Variable: %s\n\n"
          "Default value: %s\n"
          "Current value: %s\n\n"
          "Documentation:\n%s",
          name, astr_cstr(defval), curval, astr_cstr(doc));
}

DEFUN_INT("describe-variable", describe_variable)
  /*+
    Display the full documentation of a variable.
    +*/
{
  char *name;
  astr bufname, defval, doc;

  name = minibuf_read_variable_name("Describe variable: ");
  if (name == NULL)
    return FALSE;

  defval = astr_new();
  if ((doc = get_funcvar_doc(name, defval, FALSE)) == NULL) {
    astr_delete(defval);
    return FALSE;
  }

  bufname = astr_new();
  astr_afmt(bufname, "*Help: variable `%s'*", name);
  write_temp_buffer(astr_cstr(bufname), write_variable_description,
                    name, defval, get_variable(name), doc);
  astr_delete(bufname);
  astr_delete(doc);
  astr_delete(defval);

  return TRUE;
}
END_DEFUN

DEFUN_INT("describe-key", describe_key)
  /*+
    Display documentation of the command invoked by a key sequence.
    +*/
{
  char *name;
  astr bufname, doc;

  minibuf_write("Describe key:");

  if ((name = get_function_by_key_sequence()) == NULL) {
    minibuf_error("Key sequence is undefined");
    return FALSE;
  }

  minibuf_write("Key sequence runs the command `%s'", name);

  if ((doc = get_funcvar_doc(name, NULL, TRUE)) == NULL)
    return FALSE;

  bufname = astr_new();
  astr_afmt(bufname, "*Help: function `%s'*", name);
  write_temp_buffer(astr_cstr(bufname), write_function_description,
                    name, doc);
  astr_delete(bufname);
  astr_delete(doc);

  return TRUE;
}
END_DEFUN
