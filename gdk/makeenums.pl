#!/usr/bin/perl -w

# Information about the current enumeration

my $flags;			# Is enumeration a bitmask
my $seenbitshift;			# Have we seen bitshift operators?
my $prefix;			# Prefix for this enumeration
my $enumname;			# Name for this enumeration
my $firstenum = 1;		# Is this the first enumeration in file?
my @entries;			# [ $name, $val ] for each entry

sub parse_options {
    my $opts = shift;
    my @opts;

    for $opt (split /\s*,\s*/, $opts) {
	my ($key,$val) = $opt =~ /\s*(\w+)(?:=(\S+))?/;
	defined $val or $val = 1;
	push @opts, $key, $val;
    }
    @opts;
}
sub parse_entries {
    my $file = shift;

    while (<$file>) {
	# Read lines until we have no open comments
	while (m@/\*
	       ([^*]|\*(?!/))*$
	       @x) {
	    my $new;
	    defined ($new = <$file>) || die "Unmatched comment";
	    $_ .= $new;
	}
	# Now strip comments
	s@/\*(?!<)
	    ([^*]+|\*(?!/))*
	   \*/@@gx;
	
	s@\n@ @;
	
	next if m@^\s*$@;

	# Handle include files
	if (/^\#include\s*<([^>]*)>/ ) {
            my $file= "../$1";
	    open NEWFILE, $file or die "Cannot open include file $file: $!\n";
	    
	    if (parse_entries (\*NEWFILE)) {
		return 1;
	    } else {
		next;
	    }
	}
	
	if (/^\s*\}\s*(\w+)/) {
	    $enumname = $1;
	    return 1;
	}

	if (m@^\s*
              (\w+)\s*		         # name
              (?:=(                      # value
                   (?:[^,/]|/(?!\*))*
                  ))?,?\s*
              (?:/\*<		         # options 
                (([^*]|\*(?!/))*)
               >\*/)?
              \s*$
             @x) {
	    my ($name, $value, $options) = ($1,$2,$3);

	    if (!defined $flags && defined $value && $value =~ /<</) {
		$seenbitshift = 1;
	    }
	    if (defined $options) {
		my %options = parse_options($options);
		if (!defined $options{skip}) {
		    push @entries, [ $name, $options{nick} ];
		}
	    } else {
		push @entries, [ $name ];
	    }
	} else {
	    print STDERR "Can't understand: $_\n";
	}
    }
    return 0;
}


my $gen_arrays = 0;
my $gen_defs = 0;
my $gen_includes = 0;
my $gen_cfile = 0;

# Parse arguments

if (@ARGV) {
    if ($ARGV[0] eq "arrays") {
	shift @ARGV;
	$gen_arrays = 1;
    } elsif ($ARGV[0] eq "defs") {
	shift @ARGV;
	$gen_defs = 1;
    } elsif ($ARGV[0] eq "include") {
	shift @ARGV;
	$gen_includes = 1;
    } elsif ($ARGV[0] eq "cfile") {
	shift @ARGV;
	$gen_cfile = 1;
    }
}

if ($gen_defs) {
    print ";; generated by makeenums.pl  ; -*- scheme -*-\n\n";
} else {
    print "/* Generated by makeenums.pl */\n\n";
}

if ($gen_includes) {
  print "#ifndef __BDK_ENUM_TYPES_H__\n";
  print "#define __BDK_ENUM_TYPES_H__\n";
}

if ($gen_cfile) {
  print "#include \"bdk.h\"\n";
}

ENUMERATION:
while (<>) {
    if (eof) {
	close (ARGV);		# reset line numbering
	$firstenum = 1;		# Flag to print filename at next enum
    }

    if (m@^\s*typedef\s+enum\s*
           ({)?\s*
           (?:/\*<
             (([^*]|\*(?!/))*)
            >\*/)?
         @x) {
      print "\n";
	if (defined $2) {
	    my %options = parse_options($2);
	    $prefix = $options{prefix};
	    $flags = $options{flags};
	} else {
	    $prefix = undef;
	    $flags = undef;
	}
	# Didn't have trailing '{' look on next lines
	if (!defined $1) {
	    while (<>) {
		if (s/^\s*\{//) {
		    last;
		}
	    }
	}

	$seenbitshift = 0;
	@entries = ();

	# Now parse the entries
	parse_entries (\*ARGV);

	# figure out if this was a flags or enums enumeration

	if (!defined $flags) {
	    $flags = $seenbitshift;
	}

	# Autogenerate a prefix

	if (!defined $prefix) {
	    for (@entries) {
		my $name = $_->[0];
		if (defined $prefix) {
		    my $tmp = ~ ($name ^ $prefix);
		    ($tmp) = $tmp =~ /(^\xff*)/;
		    $prefix = $prefix & $tmp;
		} else {
		    $prefix = $name;
		}
	    }
	    # Trim so that it ends in an underscore
	    $prefix =~ s/_[^_]*$/_/;
	}
	
	for $entry (@entries) {
	    my ($name,$nick) = @{$entry};
            if (!defined $nick) {
 	        ($nick = $name) =~ s/^$prefix//;
	        $nick =~ tr/_/-/;
	        $nick = lc($nick);
	        @{$entry} = ($name, $nick);
            }
	}

	# Spit out the output

        my $valuename = $enumname;
        $valuename =~ s/([^A-Z])([A-Z])/$1_$2/g;
        $valuename =~ s/([A-Z][A-Z])([A-Z][0-9a-z])/$1_$2/g;
        $valuename = lc($valuename);

        my $typemacro = $enumname;
        $typemacro =~ s/([^A-Z])([A-Z])/$1_$2/g;
        $typemacro =~ s/([A-Z][A-Z])([A-Z][0-9a-z])/$1_$2/g;
        $typemacro = uc($valuename);
        $typemacro =~ s/BDK_/BDK_TYPE_/g;

	if ($gen_defs) {
	    if ($firstenum) {
		print qq(\n; enumerations from "$ARGV"\n);
		$firstenum = 0;
	    }

	    print "\n(define-".($flags ? "flags" : "enum")." $enumname";

	    for (@entries) {
		my ($name,$nick) = @{$_};
		print "\n   ($nick $name)";
	    }
	    print ")\n";

	} elsif ($gen_arrays) {

	    print "static const BtkEnumValue _${valuename}_values[] = {\n";
	    for (@entries) {
		my ($name,$nick) = @{$_};
		print qq(  { $name, "$name", "$nick" },\n);
	    }
	    print "  { 0, NULL, NULL }\n";
	    print "};\n";
	} elsif ($gen_includes) {
            print "GType ${valuename}_get_type (void);\n";
            print "#define ${typemacro} ${valuename}_get_type ()\n";
          } elsif ($gen_cfile) {
            print (<<EOF);
GType
${valuename}_get_type (void)
{
  static GType etype = 0;
  if (etype == 0)
    {
EOF
            if ($flags) {
              print "      static const GFlagsValue values[] = {\n";
            } else {
              print "      static const GEnumValue values[] = {\n";
            }
            for (@entries) {
              my ($name,$nick) = @{$_};
              print qq(        { $name, "$name", "$nick" },\n);
            }
	    print "        { 0, NULL, NULL }\n";
	    print "      };\n";

            if ($flags) {
              print "      etype = g_flags_register_static (\"$enumname\", values);\n";
            } else {
              print "      etype = g_enum_register_static (\"$enumname\", values);\n";
            }

            print (<<EOF);
    }
  return etype;
}
EOF
          }
        print "\n";
      }
  }


if ($gen_includes) {
  print "#endif /* __BDK_ENUMS_H__ */\n";
}
