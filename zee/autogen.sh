#! /bin/sh
# Run the autotools to generate configure and everything needed to run it

aclocal
autoheader
automake --foreign --add-missing
autoconf
rm -f config.cache
