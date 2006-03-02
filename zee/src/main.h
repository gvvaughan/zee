/* Main types and definitions
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
   Software Foundation, Fifth Floor, 51 Franklin Street, Boston, MA
   02111-1301, USA.  */

#ifndef MAIN_H
#define MAIN_H

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "zmalloc.h"
#include "vector.h"
#include "list.h"
#include "astr.h"

#undef TRUE
#define TRUE                            1
#undef FALSE
#define FALSE                           0

#undef min
#define min(a, b)                       ((a) < (b) ? (a) : (b))
#undef max
#define max(a, b)                       ((a) > (b) ? (a) : (b))

/*--------------------------------------------------------------------------
 * Main editor structures.
 *--------------------------------------------------------------------------*/

/* Line.
 * A line is a list whose items are astrs. The newline at the end of
 * each line is implicit.
 */
typedef struct list_s Line;

/* Point and Marker */
typedef struct {
  Line *p;                      /* Line pointer */
  size_t n;                     /* Line number */
  size_t o;                     /* Offset */
} Point;

/* A Marker points to a position in a buffer, and is updated when
   the buffer is modified */
typedef struct Marker {
  Point pt;                     /* Point position */
  struct Marker *next;   /* Used to chain all markers in the buffer */
} Marker;

/* Undo delta types */
enum {
  UNDO_REPLACE_BLOCK,           /* Replace a block of characters */
  UNDO_START_SEQUENCE,          /* Start a multi operation sequence */
  UNDO_END_SEQUENCE             /* End a multi operation sequence */
};

typedef struct Undo {
  struct Undo *next;            /* Next undo delta in list */
  int type;                     /* The type of undo delta */
  Point pt;               /* Where the undo delta is to be applied. */
  int unchanged; /* Flag indicating that reverting this undo leaves the buffer
                    in an unchanged state */
  astr text;                    /* Replacement string */
  size_t size;                  /* Block size for replace */
} Undo;

typedef struct {
  Point start;                  /* The region start */
  Point end;                    /* The region end */
  size_t size;                  /* The region size */

  /* The total number of lines ('\n' newlines) in region */
  int num_lines;
} Region;

/* Buffer flags or minor modes */

#define BFLAG_MODIFIED  (0x0001) /* The buffer has been modified */
#define BFLAG_READONLY  (0x0002) /* The buffer cannot be modified */
#define BFLAG_AUTOFILL  (0x0004) /* The buffer is in Auto Fill mode */
#define BFLAG_ISEARCH   (0x0008) /* The buffer is in Isearch loop */
#define BFLAG_ANCHORED  (0x0010) /* The mark is anchored */

/*
 * Represents a buffer.
 */
typedef struct {
  Line *lines;                  /* The lines of text */
  size_t num_lines;      /* The total number of lines in the buffer */
  Point pt;                     /* The point */
  Marker *mark;                 /* The mark */
  Marker *markers;              /* Markers */
  Undo *next_undop;     /* The undo deltas recorded for this buffer */
  Undo *last_undop;
  int flags;                    /* Buffer flags */
  astr filename;               /* The name of the file being edited */
} Buffer;

/*
 * Represents a window on the screen: a rectangular area used to
 * display a buffer.
 */
typedef struct {
  size_t topdelta; /* The buffer line displayed in the topmost window line */
  size_t fwidth, fheight; /* The formal width and height of the window
                             (space used on the terminal). */
  size_t ewidth, eheight; /* The effective width and height of the
                             window (space available for buffer display). */

} Window;

/* Return type of completion_try. */
enum {
  COMPLETION_NOTCOMPLETING,
  COMPLETION_NOTMATCHED	=	FALSE,
  COMPLETION_MATCHED =		TRUE
};

#define COMPLETION_POPPEDUP             0x2 /* Can I renumber this to 0x1? */

typedef struct {
  int flags;                    /* Flags */
  list completions;             /* The completion strings */
  list matches;                 /* The matches list */
  astr match;                   /* The current matched string */
} Completion;

typedef struct {
  list elements;                /* Elements (strings) */
  list sel;                     /* Currently selected element (pointer
                                   to elements) */
} History;

typedef struct Macro {
  vector *keys;                 /* Vector of keystrokes */
  astr name;                    /* Name of the macro */
  struct Macro *next;           /* Next macro in the list */
} Macro;

/* Type of font attributes */
typedef size_t Font;

/* Font codes
 * Designed to fit in an int, leaving room for a char underneath */
#define FONT_NORMAL		0x000
#define FONT_REVERSE		0x100

/*--------------------------------------------------------------------------
 * Keyboard handling.
 *--------------------------------------------------------------------------*/

#define GETKEY_DELAYED                  0x1
#define GETKEY_UNFILTERED               0x2

/* Default waitkey pause in ds */
#define WAITKEY_DEFAULT 20

/* Named keys */
enum {
#define X(key_sym, key_name, key_code) \
	key_sym = key_code,
#include "tbl_keys.h"
#undef X
  KBD_CANCEL = (KBD_CTRL | 'g'), /* Special key that shouldn't be in text tables */
  KBD_NOKEY = INT_MAX
};

/*--------------------------------------------------------------------------
 * Global flags.
 *--------------------------------------------------------------------------*/

/* The last command was a C-p or a C-n */
#define FLAG_DONE_CPCN                  0x0001
/* The last command was a kill */
#define FLAG_DONE_KILL                  0x0002
/* Hint for the redisplay engine: a resync is required */
#define FLAG_NEED_RESYNC                0x0004
/* Quit the editor as soon as possible */
#define FLAG_QUIT                       0x0008
/* We are defining a macro */
#define FLAG_DEFINING_MACRO             0x0010

/*--------------------------------------------------------------------------
 * Miscellaneous stuff.
 *--------------------------------------------------------------------------*/

/* Ensure PATH_MAX is defined */
#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX	_POSIX_PATH_MAX
#else
/* Guess if all else fails */
#define PATH_MAX	254
#endif
#endif

/* The type of a user command. */
typedef int (*Command)(list l);

typedef struct {
  size_t key;
  Command cmd;
} Binding;

/* Define an interactive command.
 *
 * DEF(command, doc) declares a command.
 * DEF_ARG(command, doc, arg1, ..., argn) declares a command that
 * takes arguments.
 * INT(arg) declares an integer argument.
 *
 * The C prototype for commands is Command (see above).
 *
 * To call such a command with an argument list a1, a2, ..., an, pass
 * the arguments (which should all be astrs) in a list.
 *
 * The macro `CMDCALL' can be used to call zero-argument commands.
 * The macro `CMDCALL_INT' can be used to call commands taking a
 * single integer argument.
 */
#define DEF(name, doc) \
  int F_ ## name(list l) \
  { \
    int ok = TRUE; \
    assert(l);

#define DEF_ARG(name, doc, args) \
  DEF(name, doc) \
  args

#define END_DEF \
    return ok; \
  }

#define STR(name, prompt) \
    astr name = NULL; \
    if (!list_empty(l)) \
      name = list_behead(l); \
    else if ((name = minibuf_read(astr_new(prompt), astr_new(""))) == NULL) \
      ok = CMDCALL(cancel);

#define COMMAND(name, prompt) \
    astr name = NULL; \
    if (!list_empty(l)) \
      name = list_behead(l); \
    else if ((name = minibuf_read_command_name(astr_new(prompt))) == NULL) \
      ok = CMDCALL(cancel);

#define UINT(name, prompt) \
    size_t name = 0; \
    if (!list_empty(l)) { \
      astr as = list_behead(l); \
      if ((name = strtoul(astr_cstr(as), NULL, 10)) == ULONG_MAX) \
        ok = FALSE; \
    } else do { \
      astr ms; \
      if ((ms = minibuf_read(astr_new(prompt), astr_new(""))) == NULL) { \
        ok = CMDCALL(cancel); \
        break; \
      } \
      if ((name = strtoul(astr_cstr(ms), NULL, 10)) == ULONG_MAX) \
        ding(); \
    } while (name == ULONG_MAX);

/* Call an interactive command */
#define CMDCALL(name) \
  F_ ## name(list_new())

/* Call an interactive command with an integer argument */
#define CMDCALL_UINT(name, arg) \
  F_ ## name(list_append(list_new(), astr_afmt("%d", arg)))

#endif /* !MAIN_H */
