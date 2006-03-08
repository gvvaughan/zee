/* Variables handling functions
   Copyright (c) 1997-2004 Sandro Sigala.
   Copyright (c) 2003-2006 Reuben Thomas.
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
   Software Foundation, Fifth Floor, 51 Franklin Street, Boston, MA
   02111-1301, USA.  */

#include "config.h"

#include <stdbool.h>
#include <stdlib.h>

#include "main.h"
#include "extern.h"


static list varlist;

typedef struct {
  astr key;
  astr value;
} vpair;

void init_variables(void)
{
  varlist = list_new();
}

static list variable_find(astr key)
{
  list p;

  if (key)
    for (p = list_first(varlist); p != varlist; p = list_next(p))
      if (!astr_cmp(key, ((vpair *)(p->item))->key))
        return p;

  return NULL;
}

void set_variable(astr key, astr value)
{
  if (key && value) {
    list temp = variable_find(key);

    if (temp == NULL || temp->item == NULL) {
      vpair *vp = zmalloc(sizeof(vpair));
      vp->key = key;
      list_prepend(varlist, vp);
      temp = list_first(varlist);
    }

    ((vpair *)(temp->item))->value = value;
  }
}


/*
 * Default variables values table.
 */
typedef struct {
  const char *var;              /* Variable name. */
  const char *fmt;              /* Variable format (boolean, etc.). */
  const char *val;              /* Default value. */
} var_entry;

static var_entry def_vars[] = {
#define X(var, fmt, val, doc) \
    {var, fmt, val},
#include "tbl_vars.h"
#undef X
};

static var_entry *get_variable_default(astr var)
{
  var_entry *p;
  for (p = &def_vars[0]; p < &def_vars[sizeof(def_vars) / sizeof(def_vars[0])]; p++)
    if (!astr_cmp(astr_new(p->var), var))
      return p;

  return NULL;
}

astr get_variable(astr var)
{
  var_entry *p;

  list temp = variable_find(var);
  if (temp && temp->item)
    return ((vpair *)(temp->item))->value;

  if ((p = get_variable_default(var)))
    return astr_new(p->val);

  return NULL;
}

int get_variable_number(astr var)
{
  astr as;

  if ((as = get_variable(var)))
    return atoi(astr_cstr(as));

  return 0;
}

bool get_variable_bool(astr var)
{
  astr as;

  if ((as = get_variable(var)))
    return !astr_cmp(as, astr_new("true"));

  return false;
}

astr minibuf_read_variable_name(astr msg)
{
  astr ms;
  Completion *cp = completion_new();
  var_entry *p;

  for (p = &def_vars[0]; p < &def_vars[sizeof(def_vars) / sizeof(def_vars[0])]; p++)
    list_append(cp->completions, astr_new(p->var));

  for (;;) {
    ms = minibuf_read_completion(msg, astr_new(""), cp, NULL);

    if (ms == NULL) {
      CMDCALL(edit_select_off);
      return NULL;
    }

    if (astr_len(ms) == 0) {
      minibuf_error(astr_new("No variable name given"));
      return NULL;
    } else if (get_variable(ms) == NULL) {
      minibuf_error(astr_afmt("There is no variable called `%s'", astr_cstr(ms)));
      waitkey(WAITKEY_DEFAULT);
    } else {
      minibuf_clear();
      break;
    }
  }

  return ms;
}

DEF(preferences_set_variable,
"\
Set a variable to the specified value.\
")
{
  astr var, val;

  ok = false;

  if (list_length(l) > 1) {
    var = list_behead(l);
    val = list_behead(l);
    ok = true;
  } else {
    if ((var = minibuf_read_variable_name(astr_new("Set variable: ")))) {
      var_entry *p = get_variable_default(var);
      if (!astr_cmp(astr_new(p ? p->fmt : ""), astr_new("b"))) {
        int i;
        if ((i = minibuf_read_boolean(astr_afmt("Set %s to value: ", astr_cstr(var)))) == -1)
          CMDCALL(edit_select_off); /* FIXME: val for set_variable() below... */
        else {
          val = astr_new((i == true) ? "true" : "false");
          ok = true;
        }
      } else                    /* Non-boolean variable. */
        if ((val = minibuf_read(astr_afmt("Set %s to value: ", astr_cstr(var)), astr_new(""))) == NULL)
          CMDCALL(edit_select_off);
        else
          ok = true;
    }
  }

  if (ok)
    set_variable(var, val);
}
END_DEF
