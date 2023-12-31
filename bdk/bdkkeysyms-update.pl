#!/usr/bin/env perl

# Updates http://git.bunny.org/cgit/btk+/tree/bdk/bdkkeysyms.h from upstream (X.org 7.x),
# from http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=keysymdef.h
# 
# Author  : Simos Xenitellis <simos at bunny dot org>.
# Authos  : Bastien Nocera <hadess@hadess.net>
# Version : 1.2
#
# Input   : http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=keysymdef.h
# Input   : http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=XF86keysym.h
# Output  : http://git.bunny.org/cgit/btk+/tree/bdk/bdkkeysyms.h
# 
# Notes   : It downloads keysymdef.h from the Internet, if not found locally,
# Notes   : and creates an updated bdkkeysyms.h
# Notes   : This version updates the source of bdkkeysyms.h from CVS to the GIT server.

use strict;

# Used for reading the keysymdef symbols.
my @keysymelements;

if ( ! -f "keysymdef.h" )
{
	print "Trying to download keysymdef.h from\n";
	print "http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=keysymdef.h\n";
	die "Unable to download keysymdef.h from http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=keysymdef.h\n" 
		unless system("wget -c -O keysymdef.h \"http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=keysymdef.h\"") == 0;
	print " done.\n\n";
}
else
{
	print "We are using existing keysymdef.h found in this directory.\n";
	print "It is assumed that you took care and it is a recent version\n";
	print "as found at http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob;f=keysymdef.h\n\n";
}

if ( ! -f "XF86keysym.h" )
{
	print "Trying to download XF86keysym.h from\n";
	print "http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=XF86keysym.h\n";
	die "Unable to download keysymdef.h from http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=XF86keysym.h\n" 
		unless system("wget -c -O XF86keysym.h \"http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=XF86keysym.h\"") == 0;
	print " done.\n\n";
}
else
{
	print "We are using existing XF86keysym.h found in this directory.\n";
	print "It is assumed that you took care and it is a recent version\n";
	print "as found at http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob;f=XF86keysym.h\n\n";
}

if ( -f "bdkkeysyms.h" )
{
	print "There is already a bdkkeysyms.h file in this directory. We are not overwriting it.\n";
	print "Please move it somewhere else in order to run this script.\n";
	die "Exiting...\n\n";
}

# Source: http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob;f=keysymdef.h
die "Could not open file keysymdef.h: $!\n" unless open(IN_KEYSYMDEF, "<:utf8", "keysymdef.h");

# Output: btk+/bdk/bdkkeysyms.h
die "Could not open file bdkkeysyms.h: $!\n" unless open(OUT_BDKKEYSYMS, ">:utf8", "bdkkeysyms.h");

# Output: btk+/bdk/bdkkeysyms-compat.h
die "Could not open file bdkkeysyms-compat.h: $!\n" unless open(OUT_BDKKEYSYMS_COMPAT, ">:utf8", "bdkkeysyms-compat.h");

my $LICENSE_HEADER= <<EOF;
/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2005, 2006, 2007, 2009 BUNNY Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

EOF

print OUT_BDKKEYSYMS $LICENSE_HEADER;
print OUT_BDKKEYSYMS_COMPAT $LICENSE_HEADER;

print OUT_BDKKEYSYMS<<EOF;

/*
 * File auto-generated from script http://git.bunny.org/cgit/btk+/tree/bdk/bdkkeysyms-update.pl
 * using the input file
 * http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=keysymdef.h
 * and
 * http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob_plain;f=XF86keysym.h
 */

/*
 * Modified by the BTK+ Team and others 1997-2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BDK_KEYSYMS_H__
#define __BDK_KEYSYMS_H__

/* For BTK 2, we include compatibility defines by default. */
#include <bdk/bdkkeysyms-compat.h> 

EOF

print OUT_BDKKEYSYMS_COMPAT<<EOF;
/*
 * Compatibility version of bdkkeysyms.h.
 *
 * In BTK3, keysyms changed to have a KEY_ prefix.  This is a compatibility header
 * your application can include to gain access to the old names as well.  Consider
 * porting to the new names instead.
 */

#ifndef __BDK_KEYSYMS_COMPAT_H__
#define __BDK_KEYSYMS_COMPAT_H__

EOF

while (<IN_KEYSYMDEF>)
{
	next if ( ! /^#define / );

	@keysymelements = split(/\s+/);
	die "Internal error, no \@keysymelements: $_\n" unless @keysymelements;

	$_ = $keysymelements[1];
	die "Internal error, was expecting \"XC_*\", found: $_\n" if ( ! /^XK_/ );
	
	$_ = $keysymelements[2];
	die "Internal error, was expecting \"0x*\", found: $_\n" if ( ! /^0x/ );

	my $element = $keysymelements[1];
	my $binding = $element;
	$binding =~ s/^XK_/BDK_KEY_/g;
	my $compat_binding = $element;
	$compat_binding =~ s/^XK_/BDK_/g;

	printf OUT_BDKKEYSYMS "#define %s 0x%03x\n", $binding, hex($keysymelements[2]);
	printf OUT_BDKKEYSYMS_COMPAT "#define %s 0x%03x\n", $compat_binding, hex($keysymelements[2]);
}

close IN_KEYSYMDEF;

#$bdksyms{"0"} = "0000";

# Source: http://gitweb.freedesktop.org/?p=xorg/proto/x11proto.git;a=blob;f=XF86keysym.h
die "Could not open file XF86keysym.h: $!\n" unless open(IN_XF86KEYSYM, "<:utf8", "XF86keysym.h");

while (<IN_XF86KEYSYM>)
{
	next if ( ! /^#define / );

	@keysymelements = split(/\s+/);
	die "Internal error, no \@keysymelements: $_\n" unless @keysymelements;

	$_ = $keysymelements[1];
	die "Internal error, was expecting \"XF86XK_*\", found: $_\n" if ( ! /^XF86XK_/ );

	# Work-around https://bugs.freedesktop.org/show_bug.cgi?id=11193
	if ($_ eq "XF86XK_XF86BackForward") {
		$keysymelements[1] = "XF86XK_AudioForward";
	}
	# XF86XK_Clear could end up a dupe of XK_Clear
	# XF86XK_Select could end up a dupe of XK_Select
	if ($_ eq "XF86XK_Clear") {
		$keysymelements[1] = "XF86XK_WindowClear";
	}
	if ($_ eq "XF86XK_Select") {
		$keysymelements[1] = "XF86XK_SelectButton";
	}

	# Ignore XF86XK_Q
	next if ( $_ eq "XF86XK_Q");
	# XF86XK_Calculater is misspelled, and a dupe
	next if ( $_ eq "XF86XK_Calculater");

	$_ = $keysymelements[2];
	die "Internal error, was expecting \"0x*\", found: $_\n" if ( ! /^0x/ );

	my $element = $keysymelements[1];
	my $binding = $element;
	$binding =~ s/^XF86XK_/BDK_KEY_/g;
	my $compat_binding = $element;
	$compat_binding =~ s/^XF86XK_/BDK_/g;

	printf OUT_BDKKEYSYMS "#define %s 0x%03x\n", $binding, hex($keysymelements[2]);
	printf OUT_BDKKEYSYMS_COMPAT "#define %s 0x%03x\n", $compat_binding, hex($keysymelements[2]);
}

close IN_XF86KEYSYM;


print OUT_BDKKEYSYMS<<EOF;

#endif /* __BDK_KEYSYMS_H__ */
EOF

print OUT_BDKKEYSYMS_COMPAT<<EOF;

#endif /* __BDK_KEYSYMS_COMPAT_H__ */
EOF

printf "We just finished converting keysymdef.h to bdkkeysyms.h and bdkkeysyms-compat.h\nThank you\n";
