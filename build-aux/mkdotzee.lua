-- Produce dotzee.sample
--
-- Copyright (c) 2012-2014 Free Software Foundation, Inc.
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

-- This script should be run as zee -e "loadfile ('script.lua') ()"

-- Don't note where the contents of this file comes from or that it's
-- auto-generated, because it's ugly in a user configuration file.
io.write (
  [[
-- ]] .. os.getenv ("PACKAGE_NAME") .. [[ configuration

-- Rebind keys with:
-- bind_key("key", "func")

]])

for name, var in pairs (env) do
  if not command_exists (name) then
    io.writelines ("-- " .. var.doc:gsub ("\n", "\n; "),
                   "-- Default value is " .. var.val .. ".",
                   "call_command (\"preferences-set-variable\", \"" .. name .. "\", \"" .. var.val .. "\")",
                   "")
  end
end

os.exit ()
