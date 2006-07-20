/* Global functions
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

/* basic.c ---------------------------------------------------------------- */
size_t get_goalc(void);
int goto_line(size_t to_line);
int goto_point(Point pt);

/* bind.c ----------------------------------------------------------------- */
void bind_key(size_t key, Command func);
rblist minibuf_read_command_name(rblist as);
rblist command_to_binding(Command f);
rblist binding_to_command(size_t key);
void process_key(size_t key);
void init_bindings(void);

/* buffer.c --------------------------------------------------------------- */
void buffer_new(void);
bool warn_if_readonly_buffer(void);
bool warn_if_no_mark(void);
bool calculate_the_region(Region *rp);
size_t tab_width(void);
size_t indent_width(void);
rblist copy_text_block(Point start, size_t size);

/* completion.c ----------------------------------------------------------- */
Completion *completion_new(void);
void completion_popup(Completion *cp);
bool completion_try(Completion *cp, rblist search);
bool completion_is_exact(Completion *cp, rblist search);
void completion_remove_suffix(Completion *cp);
size_t completion_remove_prefix(Completion *cp, rblist search);

/* display.c -------------------------------------------------------------- */
void resync_display(void);
void resize_window(void);
void popup_set(rblist as);
void popup_clear(void);
void popup_scroll_down_and_loop(void);
void popup_scroll_down(void);
void popup_scroll_up(void);
size_t term_width(void);
size_t term_height(void);
void term_set_size(size_t cols, size_t rows);
void term_display(void);
void term_tidy(void);
void term_print(rblist as);
void ding(void);

/* file.c ----------------------------------------------------------------- */
rblist get_home_dir(void);
rblist file_read(rblist filename);
void file_open(rblist filename);
void die(int exitcode);

/* history.c -------------------------------------------------------------- */
void add_history_element(History *hp, rblist string);
void prepare_history(History *hp);
rblist previous_history_element(History *hp);
rblist next_history_element(History *hp);

/* keys.c ----------------------------------------------------------------- */
size_t xgetkey(int mode, size_t timeout);
size_t getkey(void);
void waitkey(size_t timeout);
void ungetkey(size_t key);
rblist chordtostr(size_t key);
size_t strtochord(rblist chord);

/* killring.c ------------------------------------------------------------- */
void init_kill_ring(void);

/* line.c ----------------------------------------------------------------- */
void remove_marker(Marker *marker);
Marker *marker_new(Point pt);
Marker *point_marker(void);
Marker *get_mark(void);
void set_mark(Marker *m);
void set_mark_to_point(void);
Line *line_new(void);
Line *string_to_lines(rblist as, size_t *lines);
bool is_empty_line(void);
bool is_blank_line(void);
int following_char(void);
int preceding_char(void);
bool bobp(void);
bool eobp(void);
bool bolp(void);
bool eolp(void);
bool line_replace_text(Line **lp, size_t offset, size_t oldlen, rblist newtext, bool replace_case);
bool insert_char(int c);
void wrap_break_line(void);
bool insert_nstring(rblist as);
bool delete_nstring(size_t size, rblist *as);

/* macro.c ---------------------------------------------------------------- */
void cancel_kbd_macro(void);
void add_cmd_to_macro(void);
void add_key_to_cmd(size_t key);
void call_macro(Macro *mp);
Macro *get_macro(rblist name);

/* main.c ----------------------------------------------------------------- */
extern Window win;
extern Buffer *buf;
extern int thisflag, lastflag, uniarg;

/* minibuf.c -------------------------------------------------------------- */
void minibuf_write(rblist as);
void minibuf_error(rblist as);
rblist minibuf_read(rblist as, rblist value);
int minibuf_read_yesno(rblist as);
int minibuf_read_boolean(rblist as);
rblist minibuf_read_completion(rblist prompt, rblist value, Completion *cp, History *hp);
void minibuf_clear(void);

/* parser.c --------------------------------------------------------------- */
void cmd_eval(rblist as);
Command get_command(rblist name);
rblist get_command_name(Command cmd);
list command_list(void);

/* point.c ---------------------------------------------------------------- */
Point make_point(size_t lineno, size_t offset);
int cmp_point(Point pt1, Point pt2);
int point_dist(Point pt1, Point pt2);
int count_lines(Point pt1, Point pt2);
void swap_point(Point *pt1, Point *pt2);
Point point_min(Buffer *bp);
Point point_max(Buffer *bp);

/* term_{allegro,ncurses}.c ----------------------------------------------- */
void term_init(void);
void term_close(void);
void term_move(size_t y, size_t x);
void term_clrtoeol(void);
void term_refresh(void);
void term_clear(void);
void term_addch(int c);
void term_nl(void);
void term_attrset(size_t attrs, ...);
void term_beep(void);
size_t term_xgetkey(int mode, size_t timeout);

/* undo.c ----------------------------------------------------------------- */
void undo_save(int type, Point pt, size_t arg1, size_t arg2);
void undo_reset_unmodified(Undo *up);

/* variables.c ------------------------------------------------------------ */
void init_variables(void);
void set_variable(rblist var, rblist val);
rblist get_variable(rblist var);
int get_variable_number(rblist var);
bool get_variable_bool(rblist var);
rblist minibuf_read_variable_name(rblist msg);

/* External C functions for interactive commands -------------------------- */
#define X(cmd_name, doc) \
  bool F_ ## cmd_name(list l);
#include "tbl_funcs.h"
#undef X
