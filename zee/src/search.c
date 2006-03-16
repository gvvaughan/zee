/* Search and replace functions
   Copyright (c) 1997-2004 Sandro Sigala.
   Copyright (c) 2004 David A. Capello.
   Copyright (c) 2005-2006 Reuben Thomas.
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

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "main.h"
#include "extern.h"
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex.h"
#endif


static const char *find_err = NULL;

/* FIXME: Use PCRE instead */
static size_t find_substr(astr s1, astr as2, int bol, int eol, int backward)
{
  struct re_pattern_buffer pattern;
  struct re_registers search_regs;
  size_t ret = SIZE_MAX;
  int index;

  search_regs.num_regs = 1;
  search_regs.start = zmalloc(sizeof(regoff_t));
  search_regs.end = zmalloc(sizeof(regoff_t));

  pattern.translate = NULL;
  pattern.fastmap = NULL;
  pattern.buffer = NULL;
  pattern.allocated = 0;

  find_err = re_compile_pattern(rblist_to_string(as2), (int)rblist_length(as2), &pattern);
  if (!find_err) {
    pattern.not_bol = !bol;
    pattern.not_eol = !eol;

    if (!backward)
      index = re_search(&pattern, rblist_to_string(s1), (int)rblist_length(s1), 0, (int)rblist_length(s1),
                        &search_regs);
    else
      index = re_search(&pattern, rblist_to_string(s1), (int)rblist_length(s1), (int)rblist_length(s1), -(int)rblist_length(s1),
                        &search_regs);

    if (index >= 0) {
      if (!backward)
        ret = search_regs.end[0];
      else
        ret = search_regs.start[0];
    }
  }

  regfree(&pattern);

  return ret;
}

static int search_forward(Line *startp, size_t starto, astr as)
{
  if (rblist_length(as) > 0) {
    Line *lp;
    astr sp;

    for (lp = startp, sp = rblist_sub(lp->item, starto, rblist_length(lp->item));
         lp != list_last(buf->lines);
         lp = list_next(lp), sp = lp->item) {
      if (rblist_length(sp) > 0) {
        size_t off = find_substr(sp, as, sp == lp->item, true, false);
        if (off != SIZE_MAX) {
          while (buf->pt.p != lp)
            CMDCALL(move_next_line);
          buf->pt.o = off;
          return true;
        }
      }
    }
  }

  return false;
}

static int search_backward(Line *startp, size_t starto, astr as)
{
  if (rblist_length(as) > 0) {
    Line *lp;
    size_t s1size;

    for (lp = startp, s1size = starto;
         lp != list_first(buf->lines);
         lp = list_prev(lp), s1size = rblist_length(lp->item)) {
      astr sp = lp->item;
      if (s1size > 0) {
        size_t off = find_substr(sp, as, true, s1size == rblist_length(lp->item), true);
        if (off != SIZE_MAX) {
          while (buf->pt.p != lp)
            CMDCALL(move_previous_line);
          buf->pt.o = off;
          return true;
        }
      }
    }
  }

  return false;
}

static astr last_search = NULL;

#define ISEARCH_FORWARD		1
#define ISEARCH_BACKWARD	2

/*
 * Incremental search engine.
 */
static int isearch(int dir)
{
  int c;
  int last = true;
  astr as;
  astr pattern = rblist_from_string("");
  Point start, cur;
  Marker *old_mark;

  assert(buf->mark);
  old_mark = marker_new(buf->mark->pt);

  start = buf->pt;
  cur = buf->pt;

  /* I-search mode. */
  buf->flags |= BFLAG_ISEARCH;

  for (;;) {
    /* Make the minibuf message. */
    as = astr_afmt("%sI-search%s: %s",
              (last ? "" : "Failing "),
              (dir == ISEARCH_FORWARD) ? "" : " backward",
              rblist_to_string(pattern));

    /* Regex error. */
    if (find_err) {
      if ((strncmp(find_err, "Premature ", 10) == 0) ||
          (strncmp(find_err, "Unmatched ", 10) == 0) ||
          (strncmp(find_err, "Invalid ", 8) == 0)) {
        find_err = "incomplete input";
      }
      as = rblist_concat(as, astr_afmt(" [%s]", find_err));
      find_err = NULL;
    }

    minibuf_write(as);

    c = getkey();

    if (c == KBD_CANCEL) {
      buf->pt = start;
      thisflag |= FLAG_NEED_RESYNC;
      CMDCALL(edit_select_off);

      /* Restore old mark position. */
      assert(buf->mark);
      remove_marker(buf->mark);

      if (old_mark)
        buf->mark = marker_new(old_mark->pt);
      else
        buf->mark = old_mark;
      break;
    } else if (c == KBD_BS) {
      if (rblist_length(pattern) > 0) {
        pattern = rblist_sub(pattern, 0, rblist_length(pattern) - 1);
        buf->pt = start;
        thisflag |= FLAG_NEED_RESYNC;
      } else
        ding();
    } else if (c & KBD_CTRL && ((c & 0xff) == 'r' || (c & 0xff) == 's')) {
      /* Invert direction. */
      if ((c & 0xff) == 'r' && dir == ISEARCH_FORWARD)
        dir = ISEARCH_BACKWARD;
      else if ((c & 0xff) == 's' && dir == ISEARCH_BACKWARD)
        dir = ISEARCH_FORWARD;
      if (rblist_length(pattern) > 0) {
        /* Find next match. */
        cur = buf->pt;

        /* Save search string. */
        last_search = pattern;
      }
      else if (last_search)
        pattern = last_search;
    } else if (c & KBD_META || c & KBD_CTRL || c > KBD_TAB) {
      if (rblist_length(pattern) > 0) {
        /* Save mark. */
        set_mark_to_point();
        buf->mark->pt = start;

        /* Save search string. */
        last_search = pattern;

        minibuf_write(rblist_from_string("Mark saved when search started"));
      } else
        minibuf_clear();
      break;
    } else
      pattern = rblist_concat_char(pattern, c);

    if (rblist_length(pattern) > 0) {
      if (dir == ISEARCH_FORWARD)
        last = search_forward(cur.p, cur.o, pattern);
      else
        last = search_backward(cur.p, cur.o, pattern);
    } else
      last = true;

    if (thisflag & FLAG_NEED_RESYNC)
      resync_display();
  }

  /* done */
  buf->flags &= ~BFLAG_ISEARCH;

  if (old_mark)
    remove_marker(old_mark);

  return true;
}

DEF(edit_find,
"\
Do incremental search forward for regular expression.\n\
As you type characters, they add to the search string and are found.\n\
Type return to exit, leaving point at location found.\n\
Type C-s to search again forward, C-r to search again backward.\n\
C-g when search is successful aborts and moves point to starting point.\
")
{
  ok = isearch(ISEARCH_FORWARD);
}
END_DEF

DEF(edit_find_backward,
"\
Do incremental search backward for regular expression.\n\
As you type characters, they add to the search string and are found.\n\
Type return to exit, leaving point at location found.\n\
Type C-r to search again backward, C-s to search again forward.\n\
C-g when search is successful aborts and moves point to starting point.\
")
{
  ok = isearch(ISEARCH_BACKWARD);
}
END_DEF

static bool no_upper(astr as)
{
  size_t i;

  for (i = 0; i < rblist_length(as); i++)
    if (isupper(rblist_get(as, i)))
      return false;

  return true;
}

DEF(edit_find_and_replace,
"\
Replace occurrences of a regexp with other text.\n\
As each match is found, the user must type a character saying\n\
what to do with it.\
")
{
  int count = 0;
  bool noask = false, exitloop = false, find_no_upper;
  astr find, repl;

  if ((find = minibuf_read(rblist_from_string("Query replace string: "), rblist_from_string(""))) == NULL)
    ok = CMDCALL(edit_select_off);
  else if (rblist_length(find) == 0)
    ok = false;
  else {
    find_no_upper = no_upper(find);

    if ((repl = minibuf_read(astr_afmt("Query replace `%s' with: ", rblist_to_string(find)), rblist_from_string(""))) == NULL)
      ok = CMDCALL(edit_select_off);
    if (ok) {
      while (search_forward(buf->pt.p, buf->pt.o, find)) {
        if (!noask) {
          int c;
          if (thisflag & FLAG_NEED_RESYNC)
            resync_display();
          for (;;) {
            /* FIXME: Can we use minibuf_read_forced? */
            minibuf_write(astr_afmt("Query replacing `%s' with `%s' (y, n, !, ., q)? ", rblist_to_string(find), rblist_to_string(repl)));
            c = getkey();
            if (c == KBD_CANCEL || c == KBD_RET || c == ' ' || c == 'y' || c == 'n' ||
                c == 'q' || c == '.' || c == '!')
              goto exitloop;
            minibuf_error(rblist_from_string("Please answer y, n, !, . or q."));
            waitkey(WAITKEY_DEFAULT);
          }
        exitloop:
          minibuf_clear();

          switch (c) {
          case KBD_CANCEL: /* C-g */
            ok = CMDCALL(edit_select_off);
            /* Fall through. */
          case 'q': /* Quit immediately. */
            goto endoffunc;
          case '.': /* Replace and quit. */
            exitloop = true;
            goto replblock;
          case '!': /* Replace all without asking. */
            noask = true;
            goto replblock;
          case ' ': /* Replace. */
          case 'y':
            goto replblock;
            break;
          case 'n': /* Do not replace. */
          case KBD_RET:
          case KBD_DEL:
            goto nextmatch;
          }
        }

      replblock:
        ++count;
        undo_save(UNDO_REPLACE_BLOCK,
                  make_point(buf->pt.n, buf->pt.o - rblist_length(find)),
                  rblist_length(find), rblist_length(repl));
        if (line_replace_text(&buf->pt.p, buf->pt.o - rblist_length(find),
                              rblist_length(find), repl, find_no_upper))
          buf->flags |= BFLAG_MODIFIED;
        /* FALLTHROUGH */
      nextmatch:
        if (exitloop)
          break;
      }

    endoffunc:
      if (thisflag & FLAG_NEED_RESYNC)
        resync_display();
      term_display();

      minibuf_write(astr_afmt("Replaced %d occurrences", count));
    }
  }
}
END_DEF
