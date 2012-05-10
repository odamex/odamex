#!/usr/bin/perl

#-----------------------------------------------------------------------------
#
# $Id: upversion.pl 2223 2011-06-10 19:16:30Z dr_sean $
#
# Copyright (C) 2011 by The Odamex Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# DESCRIPTION:
#  UPVERSION - Updates the version number and copyright date on a given
#  set of source files
#
#-----------------------------------------------------------------------------

use strict;
use warnings;
use Getopt::Long;

my $sourcedir = '../..';	# dir containing the file to perform subsitutions on 
my $inputfn;		# name of file containing the substitutions to perform

my $version = ""; 	# $version should be in the form "0.5.3"

# process commandline arguments
if (@ARGV < 2) {
	print "upversion: Performs the regular expression substitutions specified in each\n";
	print "section of \"inputfile\" on the file named in the section heading.  The\n";
	print "version number specified will replace the previous version in the files acted on.\n\n";
	print "Usage: $0 -v <versionnum> -i <inputfile>\n";
	exit;
}
else {	
	GetOptions(	'v|version=s' 	=> \$version,
				'i|input=s' 	=> \$inputfn);
}

($version =~ /\d+\.\d+\.\d+/)
	or die "Error: Version number should be of the form 0.1.2\n";

# $version_short should be in the form "53"
my $version_short;
($version_short = $version) =~ s/^0*(\d*)\.(\d+)\.(\d+)/$1$2$3/; 

# $version_commas should be in the form "0,5,3,0"
my $version_commas;
($version_commas = $version) =~ s/^(\d+)\.(\d+)\.(\d+)/$1,$2,$3,0/;

# $savesig should be exactly 16 characters long, of the form "ODAMEXSAVE053   "
my $savesig;
($savesig = $version) =~ s/^(\d+)\.(\d+)\.(\d+)/ODAMEXSAVE$1$2$3/;
$savesig = sprintf "%-16s", $savesig;

# release_date should be in the form "January 1, 2011"
my ($mday, $mon, $year) = (localtime)[3..5];
my @months = qw( January February March April May June July August September October November December );
$year += 1900;	# year was stored as number of years after 1900
my $release_date = "$months[$mon] $mday, $year"; 

my $copyright_years = "2006-$year";

# copyright information
my $copyright = "Copyright © $copyright_years The Odamex Team";
my $copyright2 = "Copyright (C) $copyright_years The Odamex Team";


# parse_input
#
#   Read in the file containing the substitution definitions and call
#   the function to perform the substitution.

sub parse_input() {
	my $fn;
	my %cmdhash = ();	# stores the substitution commands

	open(INF, "< $inputfn")
		or die "Unable to open file $fn for reading; $!\n";
	while (<INF>) {
		chomp;
		next if /^#/;	# ignore comments which start a line with #

		# read the name of the file to act on
		if (/^\s*\[([^\]]+)\]\s*/) {
			$fn = $1;
		}
		# read the substitution command
		if (/^\s*(s.*)/) {
			my $value = $1;
			# store ref to an array containing $value to get around a hash
			# being limited to only storing one value per key
			push ( @{$cmdhash{$fn}}, $value );
		}
	}	
	close(INF);

	foreach $fn (sort keys %cmdhash) {
		open(CONTENT, "+< $sourcedir/$fn") 
			or die "Unable to open file $sourcedir/$fn for reading & writing: $!\n";
		my $text;
		while (<CONTENT>) {
			$text .= $_;
		}

		# save a backup copy of the file
		open(BACKUP, "> $sourcedir/$fn.orig") 
			or die "Unable to open file $sourcedir/$fn.orig for writing: $!\n";
		print BACKUP $text
			or die "Unable to write to file $sourcedir/$fn.orig: $!\n";
		close(BACKUP);
	
		my @substitutions = @{$cmdhash{$fn}}; 
		$text = substitute($text, @substitutions);

		print "Updating $sourcedir/$fn.\n"; 
		seek(CONTENT, 0, 0) 
			or die "Unable to seek to posiition 0 in file $sourcedir/$fn: !$\n";
		print CONTENT $text
			or die "Unable to write to file $sourcedir/$fn: $!\n";
		truncate(CONTENT, tell(CONTENT));	
		close(CONTENT);
	}
}


# substitute
#
#   Performs substitution on a line of text based on an array of
#   substitution regexs

sub substitute {
	($_, my @substitutions) = @_;

	foreach my $value (@substitutions) {
		(my $search, my $replace) = (split(/\|/, $value))[1,2];
		# evaluate the variables stored in $replace
		s/$search/eval('"' . $replace . '"')/e;	
	}

	return $_;
}

# main routine
parse_input();
