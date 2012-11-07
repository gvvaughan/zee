-- Disk file handling
--
-- Copyright (c) 2009-2012 Free Software Foundation, Inc.
--
-- This file is part of Zee.
--
-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 3, or (at your option)
-- any later version.
--
-- This program is distributed in the hope that it will be useful, but
-- WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
-- General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

-- FIXME: Reload when file changes on disk

function exist_file (filename)
  if posix.stat (filename) then
    return true
  end
  local _, err = posix.errno ()
  return err ~= posix.ENOENT
end

local function is_regular_file (filename)
  local st = posix.stat (filename)
  return st and st.type == "regular"
end

-- Return nonzero if file exists and can be written.
local function check_writable (filename)
  local ok = posix.euidaccess (filename, "w")
  return ok and ok >= 0
end

-- Write buffer to given file name with given mode.
alien.default.write:types ("ptrdiff_t", "int", "pointer", "size_t")
local function write_file (filename, mode)
  local h = posix.creat (filename, mode)
  if h then
    local s = get_buffer_pre_cursor (buf)
    local ret = alien.default.write (h, s.buf.buffer:topointer (), #s)
    if ret == #s then
      s = get_buffer_post_cursor (buf)
      ret = alien.default.write (h, s.buf.buffer:topointer (), #s)
      if ret == #s then
        ret = true
      end
    end

    local ret2 = posix.close (h)
    if ret == true and ret2 ~= 0 then -- writing error takes precedence over closing error
      ret = ret2
    end

    return ret
  end
end

Define ("file-save",
[[
Save buffer in visited file.
]],
  function ()
    local ret = write_file (buf.filename, "rw-rw-rw-")
    if not ret then
      return minibuf_error (string.format ("Error writing `%s'%s", buf.filename,
                                           ret == -1 and ": " .. posix.errno () or ""))
    end

    minibuf_write ("Wrote " .. buf.filename)
    buf.modified = false
    undo_set_unchanged (buf.last_undo)
  end
)

Define ("file-quit",
[[
Quit, unless there are unsaved changes.
]],
  function ()
    if buf.modified then
      return minibuf_error ("Unsaved changes: use `file-save' or `edit-revert'")
    end

    thisflag.quit = true
  end
)

alien.default.read:types ("ptrdiff_t", "int", "pointer", "size_t")
function read_file (filename)
  buf = buffer_new ()
  local len = posix.stat (filename, "size")
  local h = posix.open (filename, posix.O_RDONLY)
  if h and len then
    buf.text = AStr (len)
    if alien.default.read (h, buf.text:topointer (), len) == len then
      buf.filename = filename
      buf.readonly = not check_writable (filename)
    end
    posix.close (h)
  end
  if not buf.filename then
    return minibuf_error (string.format ("Error reading file `%s': %s"), filename, posix.errno ())
  end
end

-- Function called on unexpected error or crash (SIGSEGV).
-- Attempts to save modified buffer.
-- If doabort is true, aborts to allow core dump generation;
-- otherwise, exit.
function editor_exit (doabort)
  io.stderr:write ("Trying to save buffer (if modified)...\r\n")

  if buf and buf.modified then
    local file = buf.filename .. string.upper (PACKAGE) .. "SAVE"
    io.stderr:write (string.format ("Saving %s...\r\n", file))
    write_file (file, "rw-------")
  end

  term_close ()
  if doabort then
    posix.abort ()
  else
    posix._exit (2)
  end
end

Define ("file-suspend",
[[
Stop editor and return to superior process.
]],
  function ()
    posix.raise (posix.SIGTSTP)
  end
)

-- Signal an error, and abort any ongoing macro definition.
function ding ()
  if thisflag.defining_macro then
    cancel_macro_definition ()
  end

  if win then
    term_beep ()
  end
  -- enable call chaining with `return ding ()'
  return true
end
