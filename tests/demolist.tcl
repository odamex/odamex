#!/bin/bash
# \
exec tclsh "$0" "$@"

#
# runs ./odamex -nosound -novideo +demotest DEMONAME[.LMP] -file WADFILE.WAD
# against list of demos in the DEMOLIST file
#
# assumes file input format:
# DOOM2.WAD {PWAD.WAD DEH.DEH ...} DEMOLUMP.LMP {15eb4720 3ccc7a1 3fc7e27 800000}
#
# produces output format like:
# DOOM2.WAD DEMO1 [PASS]
# DOOM2.WAD TEST.LMP [FAIL]
#

set file [open tests/DEMOLIST r]

while { ![eof $file] } {
	
	set demo [gets $file]
	
	set iwad [lindex $demo 0]
	set pwad [lindex $demo 1]
	set lump [lindex $demo 2]
	
	set args "-nosound -novideo"
	append args " -iwad $iwad"
	if { $pwad != "" && $pwad != "."} {
		append args " -file $pwad"
	}
	append args " +demotest $lump"

	set stdout "CRASHED"
	catch {
		set stdout [eval exec ./odamex [split $args]]
	}
	
	set result [lindex [split $stdout "\n"] end]
	set expected [lindex $demo 3]

	if { $result != $expected} {
		puts "FAIL $demo | $result"
	} else {
		puts "PASS $demo | $result"
	}
}

close $file