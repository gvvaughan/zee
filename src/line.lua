-- Line-oriented editing functions
--
-- Copyright (c) 2010-2012 Free Software Foundation, Inc.
--
-- This file is part of GNU Zile.
--
-- GNU Zile is free software; you can redistribute it and/or modify it
-- under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 3, or (at your option)
-- any later version.
--
-- GNU Zile is distributed in the hope that it will be useful, but
-- WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
-- General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with GNU Zile; see the file COPYING.  If not, write to the
-- Free Software Foundation, Fifth Floor, 51 Franklin Street, Boston,
-- MA 02111-1301, USA.

function replace_estr (del, es)
  if warn_if_readonly_buffer () then
    return false
  end

  undo_save (UNDO_REPLACE_BLOCK, get_buffer_pt_o (cur_bp), del, #es.s)
  undo_nosave = true
  buffer_replace (cur_bp, get_buffer_pt_o (cur_bp), del, "", false)
  local p = 1
  while p <= #es.s do
    local next = string.find (es.s, es.eol, p)
    local line_len = (next or #es.s + 1) - p
    buffer_replace (cur_bp, get_buffer_pt_o (cur_bp), 0, string.sub (es.s, p, p + line_len - 1), false)
    assert (move_char (line_len))
    p = p + line_len
    if next then
      buffer_replace (cur_bp, get_buffer_pt_o (cur_bp), 0, get_buffer_text (cur_bp).eol, false)
      assert (move_char (1))
      cur_bp.last_line = cur_bp.last_line + 1
      thisflag.need_resync = true
      p = p + #es.eol
    end
  end
  undo_nosave = false

  return true
end

function insert_estr (es)
  return replace_estr (0, es)
end

function insert_string (s, eol)
  return insert_estr ({s = s, eol = eol or coding_eol_lf})
end

-- Replace a string at point, moving point forwards.
function replace (del, s)
  if warn_if_readonly_buffer () then
    return false
  end

  buffer_replace (cur_bp, get_buffer_pt_o (cur_bp), del, s, false)
  assert (move_char (#s))
  return true
end

-- If point is greater than fill-column, then split the line at the
-- right-most space character at or before fill-column, if there is
-- one, or at the left-most at or after fill-column, if not. If the
-- line contains no spaces, no break is made.
--
-- Return flag indicating whether break was made.
function fill_break_line ()
  local i, old_col
  local break_col = 0
  local fillcol = get_variable_number ("fill-column")
  local break_made = false

  -- Only break if we're beyond fill-column.
  if get_goalc () > fillcol then
    -- Save point.
    local m = point_marker ()

    -- Move cursor back to fill column
    old_col = cur_bp.pt.o
    while get_goalc () > fillcol + 1 do
      cur_bp.pt.o = cur_bp.pt.o - 1
    end

    -- Find break point moving left from fill-column.
    for i = cur_bp.pt.o, 1, -1 do
      if string.match (get_buffer_text (cur_bp).s[get_buffer_o (cur_bp) + i], "%s") then
        break_col = i
        break
      end
    end

    -- If no break point moving left from fill-column, find first
    -- possible moving right.
    if break_col == 0 then
      for i = get_buffer_o (cur_bp) + cur_bp.pt.o + 1, estr_end_of_line (get_buffer_text (cur_bp), get_buffer_o (cur_bp)) do
        if string.match (get_buffer_text (cur_bp).s[i], "%s") then
          break_col = i - get_buffer_o (cur_bp)
          break
        end
      end
    end

    if break_col >= 1 then -- Break line.
      cur_bp.pt.o = break_col
      execute_function ("delete-horizontal-space")
      insert_newline ()
      goto_point (get_marker_pt (m))
      break_made = true
    else -- Undo fiddling with point.
      cur_bp.pt.o = old_col
    end

    unchain_marker (m)
  end

  return break_made
end

function insert_newline ()
  return insert_string ("\n")
end

-- Insert a newline at the current position without moving the cursor.
function intercalate_newline ()
  return insert_newline () and backward_char ()
end

local function insert_expanded_tab (inschr)
  local c = get_goalc ()
  local t = tab_width (cur_bp)

  undo_save (UNDO_START_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)

  for c = t - c % t, 1, -1 do
    inschr (' ')
  end

  undo_save (UNDO_END_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
end

local function insert_tab ()
  if warn_if_readonly_buffer () then
    return false
  end

  if get_variable_bool ("indent-tabs-mode") then
    insert_char ('\t')
  else
    insert_expanded_tab (insert_char)
  end

  return true
end

local function backward_delete_char ()
  deactivate_mark ()

  if not backward_char () then
    minibuf_error ("Beginning of buffer")
    return false
  end

  delete_char ()
  return true
end

-- Indentation command
-- Go to cur_goalc () in the previous non-blank line.
local function previous_nonblank_goalc ()
  local cur_goalc = get_goalc ()

  -- Find previous non-blank line.
  while execute_function ("forward-line", -1) == leT and is_blank_line () do
  end

  -- Go to `cur_goalc' in that non-blank line.
  while not eolp () and get_goalc () < cur_goalc do
    forward_char ()
  end
end

local function previous_line_indent ()
  local cur_indent
  local m = point_marker ()

  execute_function ("previous-line")
  execute_function ("beginning-of-line")

  -- Find first non-blank char.
  while not eolp () and string.match (following_char (), "%s") do
    forward_char ()
  end

  cur_indent = get_goalc ()

  -- Restore point.
  goto_point (get_marker_pt (m))
  unchain_marker (m)

  return cur_indent
end

Defun ("indent-for-tab-command",
       {},
[[
Indent line or insert a tab.
Depending on `tab-always-indent', either insert a tab or indent.
If initial point was within line's indentation, position after
the indentation.  Else stay at same point in text.
]],
  true,
  function ()
    if get_variable_bool ("tab-always-indent") then
      return bool_to_lisp (insert_tab ())
    elseif (get_goalc () < previous_line_indent ()) then
      return execute_function ("indent-relative")
    end
  end
)

Defun ("indent-relative",
       {},
[[
Space out to under next indent point in previous nonblank line.
An indent point is a non-whitespace character following whitespace.
The following line shows the indentation points in this line.
    ^         ^    ^     ^   ^           ^      ^  ^    ^
If the previous nonblank line has no indent points beyond the
column point starts at, `tab-to-tab-stop' is done instead, unless
this command is invoked with a numeric argument, in which case it
does nothing.
]],
  true,
  function ()
    local target_goalc = 0
    local cur_goalc = get_goalc ()
    local t = tab_width (cur_bp)
    local ok = leNIL

    if warn_if_readonly_buffer () then
      return leNIL
    end

    deactivate_mark ()

    -- If we're on first line, set target to 0.
    if cur_bp.pt.n == 0 then
      target_goalc = 0
    else
      -- Find goalc in previous non-blank line.
      local m = point_marker ()

      previous_nonblank_goalc ()

      -- Now find the next blank char.
      if preceding_char () ~= '\t' or get_goalc () <= cur_goalc then
        while not eolp () and not string.match (following_char (), "%s") do
          forward_char ()
        end
      end

      -- Find next non-blank char.
      while not eolp () and string.match (following_char (), "%s") do
        forward_char ()
      end

      -- Target column.
      if not eolp () then
        target_goalc = get_goalc ()
      end
      goto_point (get_marker_pt (m))
      unchain_marker (m)
    end

    -- Insert indentation.
    undo_save (UNDO_START_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
    if target_goalc > 0 then
      -- If not at EOL on target line, insert spaces & tabs up to
      -- target_goalc; if already at EOL on target line, insert a tab.
      cur_goalc = get_goalc ()
      if cur_goalc < target_goalc then
        repeat
          if cur_goalc % t == 0 and cur_goalc + t <= target_goalc then
            ok = bool_to_lisp (insert_tab ())
          else
            ok = bool_to_lisp (insert_char (' '))
          end
          cur_goalc = get_goalc ()
        until ok ~= leT or cur_goalc >= target_goalc
      else
        ok = bool_to_lisp (insert_tab ())
      end
    else
      ok = bool_to_lisp (insert_tab ())
    end
    undo_save (UNDO_END_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
  end
)

Defun ("newline-and-indent",
       {},
[[
Insert a newline, then indent.
Indentation is done using the `indent-for-tab-command' function.
]],
  true,
  function ()
    local ret

    local ok = leNIL

    if warn_if_readonly_buffer () then
      return leNIL
    end

    deactivate_mark ()

    undo_save (UNDO_START_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
    if insert_newline () then
      local m = point_marker ()
      local pos

      -- Check where last non-blank goalc is.
      previous_nonblank_goalc ()
      pos = get_goalc ()
      local indent = pos > 0 or (not eolp () and string.match (following_char (), "%s"))
      goto_point (get_marker_pt (m))
      unchain_marker (m)
      -- Only indent if we're in column > 0 or we're in column 0 and
      -- there is a space character there in the last non-blank line.
      if indent then
        execute_function ("indent-for-tab-command")
      end
      ok = leT
    end
    undo_save (UNDO_END_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
  end
)


Defun ("delete-char",
       {"number"},
[[
Delete the following @i{n} characters (previous if @i{n} is negative).
]],
  true,
  function (n)
    return execute_with_uniarg (true, n, delete_char, backward_delete_char)
  end
)

Defun ("backward-delete-char",
       {"number"},
[[
Delete the previous @i{n} characters (following if @i{n} is negative).
]],
  true,
  function (n)
    return execute_with_uniarg (true, n, backward_delete_char, delete_char)
  end
)

Defun ("delete-horizontal-space",
       {},
[[
Delete all spaces and tabs around point.
]],
  true,
  function ()
    undo_save (UNDO_START_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)

    while not eolp () and string.match (following_char (), "%s") do
      delete_char ()
    end

    while not bolp () and string.match (preceding_char (), "%s") do
      backward_delete_char ()
    end

    undo_save (UNDO_END_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
  end
)

Defun ("just-one-space",
       {},
[[
Delete all spaces and tabs around point, leaving one space.
]],
  true,
  function ()
    undo_save (UNDO_START_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
    execute_function ("delete-horizontal-space")
    insert_char (' ')
    undo_save (UNDO_END_SEQUENCE, get_buffer_pt_o (cur_bp), 0, 0)
  end
)

Defun ("tab-to-tab-stop",
       {"number"},
[[
Insert a tabulation at the current point position into the current
buffer.
]],
  true,
  function (n)
    return execute_with_uniarg (true, n, insert_tab)
  end
)

local function newline ()
  if cur_bp.autofill and get_goalc () > get_variable_number ("fill-column") then
    fill_break_line ()
  end
  return insert_newline ()
end

Defun ("newline",
       {"number"},
[[
Insert a newline at the current point position into
the current buffer.
]],
  true,
  function (n)
    return execute_with_uniarg (true, n, newline)
  end
)

Defun ("open-line",
       {"number"},
[[
Insert a newline and leave point before it.
]],
  true,
  function (n)
    return execute_with_uniarg (true, n, intercalate_newline)
  end
)

Defun ("insert",
       {"string"},
[[
Insert the argument at point.
]],
  false,
  function (arg)
    insert_string (arg)
  end
)
