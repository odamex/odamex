#!/bin/bash
# \
exec tclsh "$0" "$@"

#
# runs ./odamex -nosound -novideo +demotest DEMONAME[.LMP] -file WADFILE.WAD
# against list of demos in stdin
#
# assumes stdin input format:
# WADFILE.WAD DEMONAME[.LMP] numbera numberb numberc numberd
#
# produces output format like:
# DOOM2.WAD DEMO1 [PASS]
# DOOM2.WAD TEST.LMP [FAIL]
#

lappend demos "DOOM2.WAD DEMO1 {15eb4720 3ccc7a1 3fc7e27 800000}"
lappend demos "DOOM2.WAD DEMO2 {cea29400 289b9c2 fece4356 600000}"
lappend demos "DOOM2.WAD DEMO3 {dca00040 fd6a4b9c ff7bee0a ff000000}"

foreach demo $demos {
	set stdout [exec odamex -nosound -novideo   \
			-iwad [lindex $demo 0]        \
			+demotest [lindex $demo 1]]

	set result [lindex [split $stdout "\n"] end]
	set expected [lindex $demo 2]

	if { $result != $expected} {
		puts "FAIL $demo | $result"
	} else {
		puts "PASS $demo | $result"
	}
}

