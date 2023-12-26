#! /bin/sh

if test "x$btk_all_c_sources" = x; then
	echo btk_all_c_sources variable not defined
	exit 1
fi

grep 'IN_FILE' ${srcdir-.}/btk.symbols | sed 's/.*(//;s/).*//' | grep __ | sort -u > expected-files
{ cd ${srcdir-.}; grep '^ *# *define __' $btk_all_c_sources; } | sed 's/.*define //;s/ *$//' | sort > actual-files

diff expected-files actual-files && rm -f expected-files actual-files
