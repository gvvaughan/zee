package="zee"
version="git-1"
source = {
  url = "git://github.com/rrthomas/zee.git",
}
description = {
  summary = "Experimental lightweight editor",
  detailed = [[
      Zee is a lightweight editor. Zee is aimed at small footprint
      systems and quick editing sessions (it starts up and shuts down
      instantly).
   ]],
  homepage = "https://github.com/rrthomas/zee/",
  license = "GPLv3+"
}
dependencies = {
  "lua == 5.2",
  "stdlib >= 28",
  "luaposix >= 5.1.23",
  "lrexlib-gnu >= 2.7.1",
  "alien >= 0.7.0",
}
build = {
  type = "command",
  build_command = "LUA=$(LUA) CPPFLAGS=-I$(LUA_INCDIR) ./bootstrap && ./configure --prefix=$(PREFIX) --libdir=$(LIBDIR) --datadir=$(LUADIR) && make clean && make",
  install_command = "make install",
  copy_directories = {}
}
