/* Incremental search and replace functions
   Copyright (c) 1997-2004 Sandro Sigala.
   Copyright (c) 2004 David A. Capello.
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

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "zee.h"
#include "extern.h"
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex.h"
#endif

static History regexp_history;

static const char *re_find_err = NULL;

static char *re_find_substr(const char *s1, size_t s1size,
			    const char *s2, size_t s2size,
			    int bol, int eol, int backward)
{
  struct re_pattern_buffer pattern;
  struct re_registers search_regs;
  char *ret = NULL;
  int index;
  reg_syntax_t old_syntax;

  old_syntax = re_set_syntax(RE_SYNTAX_EMACS);

  search_regs.num_regs = 1;
  search_regs.start = zmalloc(sizeof(regoff_t));
  search_regs.end = zmalloc(sizeof(regoff_t));

  pattern.translate = NULL;
  pattern.fastmap = NULL;
  pattern.buffer = NULL;
  pattern.allocated = 0;

  re_find_err = re_compile_pattern(s2, (int)s2size, &pattern);
  if (!re_find_err) {
    pattern.not_bol = !bol;
    pattern.not_eol = !eol;

    if (!backward)
      index = re_search(&pattern, s1, (int)s1size, 0, (int)s1size,
                        &search_regs);
    else
      index = re_search(&pattern, s1, (int)s1size, (int)s1size, -(int)s1size,
                        &search_regs);

    if (index >= 0) {
      if (!backward)
        ret = ((char *)s1) + search_regs.end[0];
      else
        ret = ((char *)s1) + search_regs.start[0];
    }
    else if (index == -1) {
      /* no match */
    }
    else {
      /* error */
    }
  }

  re_set_syntax(old_syntax);
  regfree(&pattern);

  free(search_regs.start);
  free(search_regs.end);

  return ret;
}

static void goto_linep(Line *lp)
{
  cur_bp->pt = point_min(cur_bp);
  resync_display();
  while (cur_bp->pt.p != lp)
    next_line();
}

static int search_forward(Line *startp, size_t starto, const char *s)
{
  Line *lp;
  const char *sp, *sp2;
  size_t s1size, s2size = strlen(s);

  if (s2size < 1)
    return FALSE;

  for (lp = startp; lp != cur_bp->lines; lp = list_next(lp)) {
    if (lp == startp) {
      sp = astr_char(lp->item, (ptrdiff_t)starto);
      s1size = astr_len(lp->item) - starto;
    } else {
      sp = astr_cstr(lp->item);
      s1size = astr_len(lp->item);
    }
    if (s1size < 1)
      continue;

    sp2 = re_find_substr(sp, s1size, s, s2size,
                         sp == astr_cstr(lp->item), TRUE, FALSE);

    if (sp2 != NULL) {
      goto_linep(lp);
      cur_bp->pt.o = sp2 - astr_cstr(lp->item);
      return TRUE;
    }
  }

  return FALSE;
}

static int search_backward(Line *startp, size_t starto, const char *s)
{
  Line *lp;
  const char *sp, *sp2;
  size_t s1size, ssize = strlen(s);

  if (ssize < 1)
    return FALSE;

  for (lp = startp; lp != cur_bp->lines; lp = list_prev(lp)) {
    sp = astr_cstr(lp->item);
    if (lp == startp)
      s1size = starto;
    else
      s1size = astr_len(lp->item);
    if (s1size < 1)
      continue;

    sp2 = re_find_substr(sp, s1size, s, ssize,
                         TRUE, s1size == astr_len(lp->item), TRUE);

    if (sp2 != NULL) {
      goto_linep(lp);
      cur_bp->pt.o = sp2 - astr_cstr(lp->item);
      return TRUE;
    }
  }

  return FALSE;
}

static char *last_search = NULL;

DEFUN_INT("search-forward-regexp", search_forward_regexp)
/*+
Search forward from point for regular expression REGEXP.
+*/
{
  char *ms;

  if ((ms = minibuf_read("Regexp search: ", last_search)) == NULL)
    ok = cancel();
  else if (ms[0] == '\0')
    ok = FALSE;
  else {
    if (last_search != NULL)
      free(last_search);
    last_search = zstrdup(ms);

    if (!search_forward(cur_bp->pt.p, cur_bp->pt.o, ms)) {
      minibuf_error("Failing regexp search: `%s'", ms);
      ok = FALSE;
    }
  }
}
END_DEFUN

DEFUN_INT("search-backward-regexp", search_backward_regexp)
/*+
Search backward from point for match for regular expression REGEXP.
+*/
{
  char *ms;

  if ((ms = minibuf_read("Regexp search backward: ", last_search)) == NULL)
    ok = cancel();
  if (ms[0] == '\0')
    ok = FALSE;
  else {
    if (last_search != NULL)
      free(last_search);
    last_search = zstrdup(ms);

    if (!search_backward(cur_bp->pt.p, cur_bp->pt.o, ms)) {
      minibuf_error("Failing regexp search backward: `%s'", ms);
      ok = FALSE;
    }
  }
}
END_DEFUN

#define ISEARCH_FORWARD		1
#define ISEARCH_BACKWARD	2

/*
 * Incremental search engine.
 */
static int isearch(int dir)
{
  int c;
  int last = TRUE;
  astr buf = astr_new();
  astr pattern = astr_new();
  Point start, cur;
  Marker *old_mark;

  assert(cur_wp->bp->mark);
  old_mark = marker_new(cur_wp->bp->mark->bp, cur_wp->bp->mark->pt);

  start = cur_bp->pt;
  cur = cur_bp->pt;

  /* I-search mode. */
  cur_wp->bp->flags |= BFLAG_ISEARCH;

  for (;;) {
    /* Make the minibuf message. */
    astr_truncate(buf, 0);
    astr_afmt(buf, "%sI-search%s: %s",
              (last ? "Regexp " : "Failing regexp "),
              (dir == ISEARCH_FORWARD) ? "" : " backward",
              astr_cstr(pattern));

    /* Regex error. */
    if (re_find_err) {
      if ((strncmp(re_find_err, "Premature ", 10) == 0) ||
          (strncmp(re_find_err, "Unmatched ", 10) == 0) ||
          (strncmp(re_find_err, "Invalid ", 8) == 0)) {
        re_find_err = "incomplete input";
      }
      astr_afmt(buf, " [%s]", re_find_err);
      re_find_err = NULL;
    }

    minibuf_write("%s", astr_cstr(buf));

    c = getkey();

    if (c == KBD_CANCEL) {
      cur_bp->pt = start;
      thisflag |= FLAG_NEED_RESYNC;

      /* "Quit" (also it calls ding() and stops
         recording macros). */
      cancel();

      /* Restore old mark position. */
      assert(cur_bp->mark);
      free_marker(cur_bp->mark);

      if (old_mark)
        cur_bp->mark = marker_new(old_mark->bp, old_mark->pt);
      else
        cur_bp->mark = old_mark;
      break;
    } else if (c == KBD_BS) {
      if (astr_len(pattern) > 0) {
        astr_truncate(pattern, -1);
        cur_bp->pt = start;
        thisflag |= FLAG_NEED_RESYNC;
      } else
        ding();
    } else if (c & KBD_CTL && ((c & 0xff) == 'r' || (c & 0xff) == 's')) {
      /* Invert direction. */
      if ((c & 0xff) == 'r' && dir == ISEARCH_FORWARD)
        dir = ISEARCH_BACKWARD;
      else if ((c & 0xff) == 's' && dir == ISEARCH_BACKWARD)
        dir = ISEARCH_FORWARD;
      if (astr_len(pattern) > 0) {
        /* Find next match. */
        cur = cur_bp->pt;
        /* Save search string. */
        if (last_search != NULL)
          free(last_search);
        last_search = zstrdup(astr_cstr(pattern));
      }
      else if (last_search != NULL)
        astr_cpy_cstr(pattern, last_search);
    } else if (c & KBD_META || c & KBD_CTL || c > KBD_TAB) {
      if (c == KBD_RET && astr_len(pattern) == 0)
        if (dir == ISEARCH_FORWARD)
          FUNCALL(search_forward_regexp);
        else
          FUNCALL(search_backward_regexp);
      else if (astr_len(pattern) > 0) {
        /* Save mark. */
        set_mark();
        cur_bp->mark->pt = start;

        /* Save search string. */
        if (last_search != NULL)
          free(last_search);
        last_search = zstrdup(astr_cstr(pattern));

        minibuf_write("Mark saved when search started");
      } else
        minibuf_clear();
      break;
    } else
      astr_cat_char(pattern, c);

    if (astr_len(pattern) > 0) {
      if (dir == ISEARCH_FORWARD)
        last = search_forward(cur.p, cur.o, astr_cstr(pattern));
      else
        last = search_backward(cur.p, cur.o, astr_cstr(pattern));
    } else
      last = TRUE;

    if (thisflag & FLAG_NEED_RESYNC)
      resync_display();
  }

  /* done */
  cur_wp->bp->flags &= ~BFLAG_ISEARCH;

  astr_delete(buf);
  astr_delete(pattern);

  if (old_mark)
    free_marker(old_mark);

  return TRUE;
}

DEFUN_INT("isearch-forward-regexp", isearch_forward_regexp)
/*+
Do incremental search forward for regular expression.
As you type characters, they add to the search string and are found.
Type return to exit, leaving point at location found.
Type C-s to search again forward, C-r to search again backward.
C-g when search is successful aborts and moves point to starting point.
+*/
{
  ok = isearch(ISEARCH_FORWARD);
}
END_DEFUN

DEFUN_INT("isearch-backward-regexp", isearch_backward_regexp)
/*+
Do incremental search forward for regular expression.
As you type characters, they add to the search string and are found.
Type return to exit, leaving point at location found.
Type C-r to search again backward, C-s to search again forward.
C-g when search is successful aborts and moves point to starting point.
+*/
{
  ok = isearch(ISEARCH_BACKWARD);
}
END_DEFUN

void free_search_history(void)
{
  free_history_elements(&regexp_history);
}

static int no_upper(const char *s, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++)
    if (isupper(s[i]))
      return FALSE;

  return TRUE;
}

DEFUN_INT("replace-regexp", replace_regexp)
/*+
Replace occurrences of a regexp with other text.
+*/
{
  char *find, *repl;
  int count = 0, find_no_upper;
  size_t find_len, repl_len;

  if ((find = minibuf_read("Replace string: ", "")) == NULL)
    ok = cancel();
  else if (find[0] == '\0')
    ok = FALSE;
  else {
    find_len = strlen(find);
    find_no_upper = no_upper(find, find_len);

    if ((repl = minibuf_read("Replace `%s' with: ", "", find)) == NULL)
      ok = cancel();
    else {
      repl_len = strlen(repl);

      while (search_forward(cur_bp->pt.p, cur_bp->pt.o, find)) {
        ++count;
        undo_save(UNDO_REPLACE_BLOCK,
                  make_point(cur_bp->pt.n,
                             cur_bp->pt.o - find_len),
                  strlen(find), strlen(repl));
        line_replace_text(&cur_bp->pt.p, cur_bp->pt.o - find_len,
                          find_len, repl, repl_len, find_no_upper);
      }

      if (thisflag & FLAG_NEED_RESYNC)
        resync_display();
      term_display();

      minibuf_write("Replaced %d occurrences", count);
    }
  }
}
END_DEFUN

DEFUN_INT("query-replace-regexp", query_replace_regexp)
/*+
Replace occurrences of a regexp with other text.
As each match is found, the user must type a character saying
what to do with it.
+*/
{
  char *find, *repl;
  int count = 0, noask = FALSE, exitloop = FALSE, find_no_upper;
  size_t find_len, repl_len;

  if ((find = minibuf_read("Query replace string: ", "")) == NULL)
    ok = cancel();
  else if (*find == '\0')
    ok = FALSE;
  else {
    find_len = strlen(find);
    find_no_upper = no_upper(find, find_len);

    if ((repl = minibuf_read("Query replace `%s' with: ", "", find)) == NULL)
      ok = cancel();
    if (ok) {
      repl_len = strlen(repl);

      /* Spaghetti code follows... :-( */
      while (search_forward(cur_bp->pt.p, cur_bp->pt.o, find)) {
        if (!noask) {
          int c;
          if (thisflag & FLAG_NEED_RESYNC)
            resync_display();
          for (;;) {
            minibuf_write("Query replacing `%s' with `%s' (y, n, !, ., q)? ", find, repl);
            c = getkey();
            if (c == KBD_CANCEL || c == KBD_RET || c == ' ' || c == 'y' || c == 'n' ||
                c == 'q' || c == '.' || c == '!')
              goto exitloop;
            minibuf_error("Please answer y, n, !, . or q.");
            waitkey(WAITKEY_DEFAULT);
          }
        exitloop:
          minibuf_clear();

          switch (c) {
          case KBD_CANCEL: /* C-g */
            ok = cancel();
            /* Fall through. */
          case 'q': /* Quit immediately. */
            goto endoffunc;
          case '.': /* Replace and quit. */
            exitloop = TRUE;
            goto replblock;
          case '!': /* Replace all without asking. */
            noask = TRUE;
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
                  make_point(cur_bp->pt.n,
                             cur_bp->pt.o - find_len),
                  find_len, repl_len);
        line_replace_text(&cur_bp->pt.p, cur_bp->pt.o - find_len,
                          find_len, repl, repl_len, find_no_upper);
      nextmatch:
        if (exitloop)
          break;
      }

    endoffunc:
      if (thisflag & FLAG_NEED_RESYNC)
        resync_display();
      term_display();

      minibuf_write("Replaced %d occurrences", count);
    }
  }
}
END_DEFUN