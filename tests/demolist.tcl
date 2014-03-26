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

	if { [llength $demo] == 0 } {
		#ignore blank lines
		continue;
	}
	
	set iwad [lindex $demo 0]
	set pwad [lindex $demo 1]
	set lump [lindex $demo 2]
	
	set args "-nosound -novideo"
	append args " -iwad $iwad"
	if { $pwad != "" && $pwad != "."} {
		foreach item $pwad {
			set parts [split $item .]
			if { [lindex $parts 1] == "deh" || [lindex $parts 1] == "DEH" } {
				if [file exists tests/$item] {
					append args " -deh tests/$item"
				} else {
					append args " -deh $item"
				}
			} else {
				if [file exists tests/$lump] {
					append args " -file tests/$item"
				} else {
					append args " -file $item"
				}
			}			
		}
	}
	
	if [file exists tests/$lump] {
		append args " +demotest tests/$lump"
	} else {
		append args " +demotest $lump"
	}
	append args " +logfile odamex.log"

	set demotest "CRASHED"
	catch {
		if [file exists odamex.exe] {
			eval exec odamex.exe [split $args] > tmp
		} elseif [file exists ./odamex] {
			eval exec ./odamex [split $args] > tmp
		} else {
			eval exec ./build/client/odamex [split $args] > tmp
		}
		set log [open odamex.log r]
		while { ![eof $log] } {
			set line [gets $log]
			if { [string range $line 0 8] == "demotest:" } {
				set demotest [string range $line 9 end]
			}
		}
		close $log
	}

	set result [lindex [split $demotest "\n"] end]
	set expected [lindex $demo 3]

	if { $result != $expected} {
		puts "FAIL $demo | $result"
	} else {
		puts "PASS $demo | $result"
	}
}

close $file
