/*	$Id: funcs.c,v 1.8 2004/01/21 01:42:22 dacap Exp $	*/

/*
 * Copyright (c) 1997-2003 Sandro Sigala.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zile.h"
#include "extern.h"
#include "astr.h"

int cancel(void)
{
	cur_wp->bp->markp = NULL;

	minibuf_error("Quit");

	return FALSE;
}

DEFUN("suspend-zile", suspend_zile)
/*+
Stop Zile and return to superior process.
+*/
{
	raise(SIGTSTP);

	return TRUE;
}

DEFUN("keyboard-quit", keyboard_quit)
/*+
Cancel current command.
+*/
{
	if (cur_wp->bp->markp == NULL)
		return cancel();
	cur_wp->bp->markp = NULL;
	return FALSE;
}

static char *make_buffer_flags(bufferp bp, int iscurrent)
{
	static char buf[4];

	buf[0] = iscurrent ? '.' : ' ';
	buf[1] = (bp->flags & BFLAG_MODIFIED) ? '*' : ' ';
	/*
	 * Display the readonly flag if it is set or the buffer is
	 * the current buffer, i.e. the `*Buffer List*' buffer.
	 */
	buf[2] = (bp->flags & BFLAG_READONLY || bp == cur_bp) ? '%' : ' ';
	buf[3] = '\0';

	return buf;
}

static char *make_buffer_mode(bufferp bp)
{
	static char buf[32]; /* Make sure the buffer is large enough. */

	switch (bp->mode) {
#if ENABLE_C_MODE
	case BMODE_C:
		strcpy(buf, "C");
		break;
#endif
#if ENABLE_CPP_MODE
	case BMODE_CPP:
		strcpy(buf, "C++");
		break;
#endif
#if ENABLE_CSHARP_MODE
	case BMODE_CSHARP:
		strcpy(buf, "C#");
		break;
#endif
#if ENABLE_JAVA_MODE
	case BMODE_JAVA:
		strcpy(buf, "Java");
		break;
#endif
#if ENABLE_SHELL_MODE
	case BMODE_SHELL:
		strcpy(buf, "Shell-script");
		break;
#endif
#if ENABLE_MAIL_MODE
	case BMODE_MAIL:
		strcpy(buf, "Mail");
		break;
#endif
	default:
		strcpy(buf, "Text");
	}

	if (bp->flags & BFLAG_FONTLOCK)
		strcat(buf, " Font");

	if (bp->flags & BFLAG_AUTOFILL)
		strcat(buf, " Fill");

	return buf;
}

static void print_buf(bufferp old_bp, bufferp bp)
{
	char buf[80];

	if (bp->name[0] == ' ')
		return;

	bprintf("%3s %-16s %6u  %-13s",
		make_buffer_flags(bp, old_bp == bp),
		bp->name,
		calculate_buffer_size(bp),
		make_buffer_mode(bp));
	if (bp->filename != NULL)
		insert_string(shorten_string(buf, bp->filename, 40));
#if 0
	bprintf(" [F: %o]", bp->flags);
#endif
	insert_newline();
}

void write_temp_buffer(const char *name, void (*func)(va_list ap), ...)
{
	windowp wp, old_wp = cur_wp;
	va_list ap;

	if ((wp = find_window(name)) == NULL) {
		bufferp bp;
		cur_wp = popup_window();
		cur_bp = cur_wp->bp;
		bp = find_buffer(name, TRUE);
		switch_to_buffer(bp);
	} else {
		cur_wp = wp;
		cur_bp = wp->bp;
	}

	zap_buffer_content();
	cur_bp->flags = BFLAG_NEEDNAME | BFLAG_NOSAVE | BFLAG_NOUNDO
		| BFLAG_NOEOB;
	set_temporary_buffer(cur_bp);

	va_start(ap, func);
	func(ap);
	va_end(ap);

	gotobob();

	cur_bp->flags |= BFLAG_READONLY;

	cur_wp = old_wp;
	cur_bp = old_wp->bp;
}

static void write_buffers_list(va_list ap)
{
	windowp old_wp = va_arg(ap, windowp);
	bufferp bp;

	bprintf(" MR Buffer          Size  Mode	 File\n");
	bprintf(" -- ------          ----  ----	 ----\n");

	/* Print non-temporary buffers. */
	bp = old_wp->bp;
	do {
		if (!(bp->flags & BFLAG_TEMPORARY))
			print_buf(old_wp->bp, bp);
		if ((bp = bp->next) == NULL)
			bp = head_bp;
	} while (bp != old_wp->bp);

	/* Print temporary buffers. */
	bp = old_wp->bp;
	do {
		if (bp->flags & BFLAG_TEMPORARY)
			print_buf(old_wp->bp, bp);
		if ((bp = bp->next) == NULL)
			bp = head_bp;
	} while (bp != old_wp->bp);
}

DEFUN("list-buffers", list_buffers)
/*+
Display a list of names of existing buffers.
The list is displayed in a buffer named `*Buffer List*'.
Note that buffers with names starting with spaces are omitted.

The M column contains a * for buffers that are modified.
The R column contains a % for buffers that are read-only.
+*/
{
	write_temp_buffer("*Buffer List*", write_buffers_list, cur_wp);
	return TRUE;
}

DEFUN("overwrite-mode", overwrite_mode)
/*+
In overwrite mode, printing characters typed in replace existing text
on a one-for-one basis, rather than pushing it to the right.  At the
end of a line, such characters extend the line.
C-q still inserts characters in overwrite mode; this
is supposed to make it easier to insert characters when necessary.
+*/
{
	if (cur_bp->flags & BFLAG_OVERWRITE)
		cur_bp->flags &= ~BFLAG_OVERWRITE;
	else
		cur_bp->flags |= BFLAG_OVERWRITE;

	return TRUE;
}

DEFUN("toggle-read-only", toggle_read_only)
/*+
Change whether this buffer is visiting its file read-only.
+*/
{
	if (cur_bp->flags & BFLAG_READONLY)
		cur_bp->flags &= ~BFLAG_READONLY;
	else
		cur_bp->flags |= BFLAG_READONLY;

	return TRUE;
}

DEFUN("auto-fill-mode", auto_fill_mode)
/*+
Toggle Auto Fill mode.
In Auto Fill mode, inserting a space at a column beyond `fill-column'
automatically breaks the line at a previous space.
+*/
{
	if (cur_bp->flags & BFLAG_AUTOFILL)
		cur_bp->flags &= ~BFLAG_AUTOFILL;
	else
		cur_bp->flags |= BFLAG_AUTOFILL;

	return TRUE;
}

DEFUN("text-mode", text_mode)
/*+
Turn on the mode for editing text intended for humans to read.
+*/
{
	cur_bp->mode = BMODE_TEXT;
	if (cur_bp->flags & BFLAG_FONTLOCK)
		FUNCALL(font_lock_mode);
	if (lookup_bool_variable("text-mode-auto-fill"))
		cur_bp->flags |= BFLAG_AUTOFILL;

	return TRUE;
}

#if ENABLE_C_MODE
DEFUN("c-mode", c_mode)
/*+
Turn on the mode for editing K&R and ANSI/ISO C code.
+*/
{
	cur_bp->mode = BMODE_C;
	if (lookup_bool_variable("auto-font-lock") &&
	    !(cur_bp->flags & BFLAG_FONTLOCK))
		FUNCALL(font_lock_mode);

	return TRUE;
}
#endif

#if ENABLE_CPP_MODE
DEFUN("c++-mode", cpp_mode)
/*+
Turn on the mode for editing ANSI/ISO C++ code.
+*/
{
	cur_bp->mode = BMODE_CPP;
	if (lookup_bool_variable("auto-font-lock") &&
	    !(cur_bp->flags & BFLAG_FONTLOCK))
		FUNCALL(font_lock_mode);

	return TRUE;
}
#endif

#if ENABLE_CSHARP_MODE
DEFUN("c#-mode", csharp_mode)
/*+
Turn on the mode for editing C# (C sharp) code.
+*/
{
	cur_bp->mode = BMODE_CSHARP;
	if (lookup_bool_variable("auto-font-lock") &&
	    !(cur_bp->flags & BFLAG_FONTLOCK))
		FUNCALL(font_lock_mode);

	return TRUE;
}
#endif

#if ENABLE_JAVA_MODE
DEFUN("java-mode", java_mode)
/*+
Turn on the mode for editing Java code.
+*/
{
	cur_bp->mode = BMODE_JAVA;
	if (lookup_bool_variable("auto-font-lock") &&
	    !(cur_bp->flags & BFLAG_FONTLOCK))
		FUNCALL(font_lock_mode);

	return TRUE;
}
#endif

#if ENABLE_SHELL_MODE
DEFUN("shell-script-mode", shell_script_mode)
/*+
Turn on the mode for editing shell script code.
+*/
{
	cur_bp->mode = BMODE_SHELL;
	if (lookup_bool_variable("auto-font-lock") &&
	    !(cur_bp->flags & BFLAG_FONTLOCK))
		FUNCALL(font_lock_mode);

	return TRUE;
}
#endif

#if ENABLE_MAIL_MODE
DEFUN("mail-mode", mail_mode)
/*+
Turn on the mode for editing emails.
+*/
{
	cur_bp->mode = BMODE_MAIL;
	if (lookup_bool_variable("auto-font-lock") &&
	    !(cur_bp->flags & BFLAG_FONTLOCK))
		FUNCALL(font_lock_mode);
	if (lookup_bool_variable("mail-mode-auto-fill"))
		cur_bp->flags |= BFLAG_AUTOFILL;

	return TRUE;
}
#endif

DEFUN("set-fill-column", set_fill_column)
/*+
Set the fill column.
If an argument value is passed, set the `fill-column' variable with
that value, otherwise with the current column value.
+*/
{
	if (uniarg > 1)
		cur_bp->fill_column = uniarg;
	else if (cur_wp->pointo > 1)
		cur_bp->fill_column = cur_wp->pointo + 1;
	else {
		minibuf_error("Invalid fill column");
		return FALSE;
	}

	return TRUE;
}

int set_mark_command(void)
{
	cur_bp->markp = cur_wp->pointp;
	cur_bp->marko = cur_wp->pointo;

	minibuf_write("Mark set");

	return TRUE;
}

DEFUN("set-mark-command", set_mark_command)
/*+
Set mark at where point is.
+*/
{
	thisflag |= FLAG_HIGHLIGHT_REGION | FLAG_HIGHLIGHT_REGION_STAYS;
	return set_mark_command();
}

DEFUN("exchange-point-and-mark", exchange_point_and_mark)
/*+
Put the mark where point is now, and point where the mark is now.
+*/
{
	struct region r;
	linep swapp;
	int swapo;

	if (warn_if_no_mark())
		return FALSE;

	calculate_region(&r);

	/*
	 * Increment or decrement the current line according to the
	 * mark position.
	 */
	if (r.startp == cur_wp->pointp)
		cur_wp->pointn += r.num_lines;
	else
		cur_wp->pointn -= r.num_lines;

	/*
	 * Swap the point and the mark.
	 */
	swapp = cur_bp->markp;
	swapo = cur_bp->marko;
	cur_bp->markp = cur_wp->pointp;
	cur_bp->marko = cur_wp->pointo;
	cur_wp->pointp = swapp;
	cur_wp->pointo = swapo;

	thisflag |= FLAG_NEED_RESYNC | FLAG_HIGHLIGHT_REGION_STAYS;

	return TRUE;
}

DEFUN("mark-whole-buffer", mark_whole_buffer)
/*+
Put point at beginning and mark at end of buffer.
+*/
{
	gotoeob();
	set_mark_command();
	gotobob();

	thisflag |= FLAG_HIGHLIGHT_REGION | FLAG_HIGHLIGHT_REGION_STAYS;

	return TRUE;
}

static int quoted_insert_octal(int c1)
{
	int c2, c3;
	do {
		c2 = cur_tp->xgetkey(GETKEY_DELAYED | GETKEY_NONFILTERED, 500);
		if (c2 == KBD_CANCEL)
			return FALSE;
		minibuf_write("C-q %d-", c1 - '0');
	} while (c2 == KBD_NOKEY);

	if (!isdigit(c2) || c2 - '0' >= 8) {
		insert_char_in_insert_mode(c1 - '0');
		insert_char_in_insert_mode(c2);
		return TRUE;
	}

	do {
		c3 = cur_tp->xgetkey(GETKEY_DELAYED | GETKEY_NONFILTERED, 500);
		if (c3 == KBD_CANCEL)
			return FALSE;
		minibuf_write("C-q %d %d-", c1 - '0', c2 - '0');
	} while (c3 == KBD_NOKEY);

	if (!isdigit(c3) || c3 - '0' >= 8) {
		insert_char_in_insert_mode((c1 - '0') * 8 + (c2 - '0'));
		insert_char_in_insert_mode(c3);
		return TRUE;
	}

	insert_char_in_insert_mode((c1 - '8') * 64 + (c2 - '0') * 8 + (c3 - '0'));

	return TRUE;
}

DEFUN("quoted-insert", quoted_insert)
/*+
Read next input character and insert it.
This is useful for inserting control characters.
You may also type up to 3 octal digits, to insert a character with that code.
+*/
{
	int c;

	do {
		c = cur_tp->xgetkey(GETKEY_DELAYED | GETKEY_NONFILTERED, 500);
		minibuf_write("C-q-");
	} while (c == KBD_NOKEY);

	if (isdigit(c) && c - '0' < 8)
		quoted_insert_octal(c);
	else if (c == '\r')
		insert_newline();
	else
		insert_char_in_insert_mode(c);

	minibuf_clear();

	return TRUE;
}

int universal_argument(int keytype, int xarg)
{
	char buf[128];
	int arg = 4, neg = 1, i = 0, compl = 0;
	int c;

	if (keytype == KBD_META) {
		strcpy(buf, "ESC ");
		arg = xarg;
		++i;
		goto gotesc;
	} else
		strcpy(buf, "C-u -");
	for (;;) {
		c = do_completion(buf, &compl);
		if (c == KBD_CANCEL)
			return cancel();
		else if (c == (KBD_CTL | 'u')) {
			if (arg * 4 < arg)
				arg = 4;
			else
				arg *= 4;
			i = 0;
		}
		else if (c == '-' && i == 0)
			neg = -neg;
		else if (c > 255 || !isdigit(c)) {
			cur_tp->ungetkey(c);
			break;
		} else {
			if (i == 0)
				arg = c - '0';
			else
				arg = arg * 10 + c - '0';
			++i;
		}
	gotesc:
		if (keytype == KBD_META)
			sprintf(buf, "ESC %d", arg * neg);
		else
			sprintf(buf, "C-u %d", arg * neg);
	}

	last_uniarg = arg * neg;

	thisflag |= FLAG_SET_UNIARG;

	minibuf_clear();

	return TRUE;
}

DEFUN("universal-argument", universal_argument)
/*+
Begin a numeric argument for the following command.
Digits or minus sign following C-u make up the numeric argument.
C-u following the digits or minus sign ends the argument.
C-u without digits or minus sign provides 4 as argument.
Repeating C-u without digits or minus sign multiplies the argument
by 4 each time.
+*/
{
	return universal_argument(KBD_CTL | 'u', 0);
}

#define TAB_TABIFY	1
#define TAB_UNTABIFY	2

static void edit_tab_line(linep *lp, int lineno, int offset, int size, int action)
{
	char *src, *dest;

	if (size == 0)
		return;
	assert(size >= 1);

	src = (char *)zmalloc(size + 1);
	dest = (char *)zmalloc(size * cur_bp->tab_width + 1);
	strncpy(src, (*lp)->text + offset, size);
	src[size] = '\0';

	if (action == TAB_UNTABIFY)
		untabify_string(dest, src, offset, cur_bp->tab_width);
	else
		tabify_string(dest, src, offset, cur_bp->tab_width);

	if (strcmp(src, dest) != 0) {
		undo_save(UNDO_REPLACE_BLOCK, lineno, offset, size, strlen(dest));
		line_replace_text(lp, offset, size, dest);
	}
	free(src);
	free(dest);
}

static int edit_tab_region(int action)
{
	struct region r;
	linep lp;
	int lineno;

	if (warn_if_readonly_buffer() || warn_if_no_mark())
		return FALSE;

	calculate_region(&r);
	if (r.size == 0)
		return TRUE;

	undo_save(UNDO_START_SEQUENCE, r.startn, r.starto, 0, 0);
	for (lp = r.startp, lineno = r.startn;; lp = lp->next, ++lineno) {
		if (lp == r.startp) {
			if (lp == r.endp) /* Region on a sole line. */
				edit_tab_line(&lp, lineno, r.starto,
					      r.endo - r.starto, action);
			else /* Region is multi-line. */
				edit_tab_line(&lp, lineno, r.starto, lp->size,
					      action);
		} else if (lp == r.endp) /* Last line of multi-line region. */
			edit_tab_line(&lp, lineno, 0, r.endo, action);
		else /* Middle line of multi-line region. */
			edit_tab_line(&lp, lineno, r.starto,
				      lp->size - r.starto, action);
		if (lp == r.endp)
			break;
	}
	undo_save(UNDO_END_SEQUENCE, r.startn, r.starto, 0, 0);

	cur_wp->bp->markp = NULL;

	return TRUE;
}

DEFUN("tabify", tabify)
/*+
Convert multiple spaces in region to tabs when possible.
A group of spaces is partially replaced by tabs
when this can be done without changing the column they end at.
The variable `tab-width' controls the spacing of tab stops.
+*/
{
	if (edit_tab_region(TAB_TABIFY)) {
		/* Avoid pointing over the end of the line. */
		cur_wp->pointo = 0;
		return TRUE;
	}
	return FALSE;
}

DEFUN("untabify", untabify)
/*+
Convert all tabs in region to multiple spaces, preserving columns.
The variable `tab-width' controls the spacing of tab stops.
+*/
{
	return edit_tab_region(TAB_UNTABIFY);
}

DEFUN("back-to-indentation", back_to_indentation)
/*+
Move point to the first non-whitespace character on this line.
+*/
{
	cur_wp->pointo = 0;
	while (cur_wp->pointo < cur_wp->pointp->size) {
		if (!isspace(cur_wp->pointp->text[cur_wp->pointo]))
			break;
		++cur_wp->pointo;
	}
	thisflag |= FLAG_HIGHLIGHT_REGION_STAYS;
	return TRUE;
}

DEFUN("transpose-chars", transpose_chars)
/*+
Interchange characters around point, moving forward one character.
If at end of line, the previous two chars are exchanged.
+*/
{
}

DEFUN("transpose-words", transpose_words)
/*+
Interchange words around point, leaving point at end of them.
+*/
{
}

DEFUN("transpose-sexps", transpose_sexps)
/*+
Like M-t but applies to sexps.
+*/
{
}

DEFUN("transpose-lines", transpose_lines)
/*+
Exchange current line and previous line, leaving point after both.
With argument ARG, takes previous line and moves it past ARG lines.
With argument 0, interchanges line point is in with line mark is in.
+*/
{
}

#define ISWORDCHAR(c)	(isalnum(c) || c == '$')

static int forward_word(void)
{
	int gotword = FALSE;
	for (;;) {
		while (cur_wp->pointo <= cur_wp->pointp->size) {
			int c = cur_wp->pointp->text[cur_wp->pointo];
			if (!ISWORDCHAR(c)) {
				if (gotword)
					return TRUE;
			} else
				gotword = TRUE;
			++cur_wp->pointo;
		}
		cur_wp->pointo = cur_wp->pointp->size;
		if (!next_line())
			return FALSE;
		cur_wp->pointo = 0;
	}
	return FALSE;
}

DEFUN("forward-word", forward_word)
/*+
Move point forward one word (backward if the argument is negative).
With argument, do this that many times.
+*/
{
	int uni;

	thisflag |= FLAG_HIGHLIGHT_REGION_STAYS;

	if (uniarg < 0)
		return FUNCALL_ARG(backward_word, -uniarg);

	for (uni = 0; uni < uniarg; ++uni)
		if (!forward_word())
			return FALSE;

	return TRUE;
}

static int backward_word(void)
{
	int gotword = FALSE;
	for (;;) {
		if (cur_wp->pointo == 0) {
			if (!previous_line())
				return FALSE;
			cur_wp->pointo = cur_wp->pointp->size;
		}
		while (cur_wp->pointo > 0) {
			int c = cur_wp->pointp->text[cur_wp->pointo - 1];
			if (!ISWORDCHAR(c)) {
				if (gotword)
					return TRUE;
			} else
				gotword = TRUE;
			--cur_wp->pointo;
		}
		if (gotword)
			return TRUE;
	}
	return FALSE;
}

DEFUN("backward-word", backward_word)
/*+
Move backward until encountering the end of a word (forward if the
argument is negative).
With argument, do this that many times.
+*/
{
	int uni;

	thisflag |= FLAG_HIGHLIGHT_REGION_STAYS;

	if (uniarg < 0)
		return FUNCALL_ARG(forward_word, -uniarg);

	for (uni = 0; uni < uniarg; ++uni)
		if (!backward_word())
			return FALSE;

	return TRUE;
}

DEFUN("mark-word", mark_word)
/*+
Set mark argument words away from point.
+*/
{
        return FALSE;
}

DEFUN("backward-sentence", backward_sentence)
/*+
Move backward to start of sentence.  With argument N, do it N times.
+*/
{
        return FALSE;
}

DEFUN("forward-sentence", forward_sentence)
/*+
Move forward to next sentence end.  With argument N, do it N times.
+*/
{
        return FALSE;
}

DEFUN("kill-sentence", kill_sentence)
/*+
Kill from point to end of sentence.  With argument N, do it N times.
+*/
{
        return FALSE;
}

DEFUN("backward-kill-sentence", backward_kill_sentence)
/*+
Kill back from point to start of sentence.  With argument N, do it N times.
+*/
{
        return FALSE;
}

DEFUN("backward-paragraph", backward_paragraph)
/*+
Move backward to start of paragraph.  With argument N, do it N times.
+*/
{
        return FALSE;
}

DEFUN("forward-paragraph", forward_paragraph)
/*+
Move forward to end of paragraph.  With argument N, do it N times.
+*/
{
        return FALSE;
}

DEFUN("mark-paragraph", mark_paragraph)
/*+
Put point at beginning of this paragraph, mark at end.
The paragraph marked is the one that contains point or follows point.
+*/
{
        return FALSE;
}

DEFUN("fill-paragraph", fill_paragraph)
/*+
Fill paragraph at or after point.
+*/
{
        return FALSE;
}

#define UPPERCASE		1
#define LOWERCASE		2
#define CAPITALIZE		3

static int setcase_word(int rcase)
{
	int i, size, gotword;

	if (!forward_word())
		return FALSE;
	if (!backward_word())
		return FALSE;

	i = cur_wp->pointo;
	while (i < cur_wp->pointp->size) {
		if (!ISWORDCHAR(cur_wp->pointp->text[i]))
			break;
		++i;
	}
	size = i-cur_wp->pointo;
	if (size > 0)
		undo_save(UNDO_REPLACE_BLOCK, cur_wp->pointn, cur_wp->pointo, size, size);

	gotword = FALSE;
	while (cur_wp->pointo < cur_wp->pointp->size) {
		char *p = &cur_wp->pointp->text[cur_wp->pointo];
		if (!ISWORDCHAR(*p))
			break;
		if (isalpha(*p)) {
			int oldc = *p, newc;
			if (rcase == UPPERCASE)
				newc = toupper(oldc);
			else if (rcase == LOWERCASE)
				newc = tolower(oldc);
			else { /* rcase == CAPITALIZE */
				if (!gotword)
					newc = toupper(oldc);
				else
					newc = tolower(oldc);
			}
			if (oldc != newc) {
				*p = newc;
			}
		}
		gotword = TRUE;
		++cur_wp->pointo;
	}

	cur_bp->flags |= BFLAG_MODIFIED;

#if ENABLE_NONTEXT_MODES
	if (cur_bp->flags & BFLAG_FONTLOCK)
		font_lock_reset_anchors(cur_bp, cur_wp->pointp);
#endif

	return TRUE;
}

DEFUN("downcase-word", downcase_word)
/*+
Convert following word (or argument N words) to lower case, moving over.
+*/
{
	int uni, ret = TRUE;

	undo_save(UNDO_START_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);
	for (uni = 0; uni < uniarg; ++uni)
		if (!setcase_word(LOWERCASE)) {
			ret = FALSE;
			break;
		}
	undo_save(UNDO_END_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);

	return ret;
}

DEFUN("upcase-word", upcase_word)
/*+
Convert following word (or argument N words) to upper case, moving over.
+*/
{
	int uni, ret = TRUE;

	undo_save(UNDO_START_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);
	for (uni = 0; uni < uniarg; ++uni)
		if (!setcase_word(UPPERCASE)) {
			ret = FALSE;
			break;
		}
	undo_save(UNDO_END_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);

	return ret;
}

DEFUN("capitalize-word", capitalize_word)
/*+
Capitalize the following word (or argument N words), moving over.
This gives the word(s) a first character in upper case and the rest
lower case.
+*/
{
	int uni, ret = TRUE;

	undo_save(UNDO_START_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);
	for (uni = 0; uni < uniarg; ++uni)
		if (!setcase_word(CAPITALIZE)) {
			ret = FALSE;
			break;
		}
	undo_save(UNDO_END_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);

	return ret;
}

/*
 * Set the region case.
 */
static int setcase_region(int rcase)
{
	struct region r;
	linep lp;
	char *p;
	int size;

	if (warn_if_readonly_buffer() || warn_if_no_mark())
		return FALSE;

	calculate_region(&r);
	size = r.size;

	undo_save(UNDO_REPLACE_BLOCK, r.startn, r.starto, size, size);

	lp = r.startp;
	p = lp->text + r.starto;
	while (size--) {
		if (p < lp->text + lp->size) {
			if (rcase == UPPERCASE)
				*p = toupper(*p);
			else
				*p = tolower(*p);
			++p;
		} else {
			lp = lp->next;
			p = lp->text;
		}
	}

	cur_bp->flags |= BFLAG_MODIFIED;

	return TRUE;
}

DEFUN("upcase-region", upcase_region)
/*+
Convert the region to upper case.
+*/
{
	return setcase_region(UPPERCASE);
}

DEFUN("downcase-region", downcase_region)
/*+
Convert the region to lower case.
+*/
{
	return setcase_region(LOWERCASE);
}

static void write_shell_output(va_list ap)
{
	astr out = va_arg(ap, astr);

	insert_string((char *)astr_cstr(out));
}

DEFUN("shell-command", shell_command)
/*+
Reads a line of text using the minibuffer and creates an inferior shell
to execute the line as a command.
Standard input from the command comes from the null device.  If the
shell command produces any output, the output goes to a Zile buffer
named `*Shell Command Output*', which is displayed in another window
but not selected.
If the output is one line, it is displayed in the echo area.
A numeric argument, as in `M-1 M-!' or `C-u M-!', directs this
command to insert any output into the current buffer.
+*/
{
	char *ms;
	FILE *pipe;
	astr out, s;
	int lines = 0;
	astr cmd;

	if ((ms = minibuf_read("Shell command: ", "")) == NULL)
		return cancel();
	if (ms[0] == '\0')
		return FALSE;

	cmd = astr_new();
	astr_fmt(cmd, "%s 2>&1 </dev/null", ms);
	if ((pipe = popen(astr_cstr(cmd), "r")) == NULL) {
		minibuf_error("Cannot open pipe to process");
		return FALSE;
	}
	astr_delete(cmd);

	out = astr_new();
	s = astr_new();
	while (astr_fgets(s, pipe) != NULL) {
		++lines;
		astr_append(out, s);
		astr_append_char(out, '\n');
	}
	astr_delete(s);
	pclose(pipe);

	if (lines == 0)
		minibuf_write("(Shell command succeeded with no output)");
	else { /* lines >= 1 */
		if (lastflag & FLAG_SET_UNIARG)
			insert_string((char *)astr_cstr(out));
		else {
			if (lines > 1)
				write_temp_buffer("*Shell Command Output*",
						  write_shell_output, out);
			else /* lines == 1 */
				minibuf_write("%s", astr_cstr(out));
		}
	}
	astr_delete(out);

	return TRUE;
}

DEFUN("shell-command-on-region", shell_command_on_region)
/*+
Reads a line of text using the minibuffer and creates an inferior shell
to execute the line as a command; passes the contents of the region as
input to the shell command.
If the shell command produces any output, the output goes to a Zile buffer
named `*Shell Command Output*', which is displayed in another window
but not selected.
If the output is one line, it is displayed in the echo area.
A numeric argument, as in `M-1 M-|' or `C-u M-|', directs output to the
current buffer, then the old region is deleted first and the output replaces
it as the contents of the region.
+*/
{
	char *ms;
	FILE *pipe;
	astr out, s;
	int lines = 0;
	astr cmd;
	char tempfile[] = P_tmpdir "/zileXXXXXX";

	if ((ms = minibuf_read("Shell command: ", "")) == NULL)
		return cancel();
	if (ms[0] == '\0')
		return FALSE;

	cmd = astr_new();
	if (cur_bp->markp != NULL) { /* Region selected; filter text. */
		struct region r;
		char *p;
		int fd;

		fd = mkstemp(tempfile);
		if (fd == -1) {
			minibuf_error("Cannot open temporary file");
			return FALSE;
		}

		calculate_region(&r);
		p = copy_text_block(r.startn, r.starto, r.size);
		write(fd, p, r.size);
		free(p);

		close(fd);

		astr_fmt(cmd, "%s 2>&1 <%s", ms, tempfile);
	} else
		astr_fmt(cmd, "%s 2>&1 </dev/null", ms);

	if ((pipe = popen(astr_cstr(cmd), "r")) == NULL) {
		minibuf_error("Cannot open pipe to process");
		return FALSE;
	}
	astr_delete(cmd);

	out = astr_new();
	s = astr_new();
	while (astr_fgets(s, pipe) != NULL) {
		++lines;
		astr_append(out, s);
		astr_append_char(out, '\n');
	}
	astr_delete(s);
	pclose(pipe);
	if (cur_bp->markp != NULL)
		remove(tempfile);

	if (lines == 0)
		minibuf_write("(Shell command succeeded with no output)");
	else { /* lines >= 1 */
		if (lastflag & FLAG_SET_UNIARG) {
			undo_save(UNDO_START_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);
			if (cur_bp->markp != NULL) {
				struct region r;
				calculate_region(&r);
				if (cur_wp->pointp != r.startp || r.starto != cur_wp->pointo)
					FUNCALL(exchange_point_and_mark);
				undo_save(UNDO_INSERT_BLOCK, cur_wp->pointn, cur_wp->pointo, r.size, 0);
				undo_nosave = TRUE;
				while (r.size--)
					FUNCALL(delete_char);
				undo_nosave = FALSE;
			}
			insert_string((char *)astr_cstr(out));
			undo_save(UNDO_END_SEQUENCE, cur_wp->pointn, cur_wp->pointo, 0, 0);
		} else {
			if (lines > 1)
				write_temp_buffer("*Shell Command Output*",
						  write_shell_output, out);
			else /* lines == 1 */
				minibuf_write("%s", astr_cstr(out));
		}
	}
	astr_delete(out);

	return TRUE;
}
