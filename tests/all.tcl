#!/bin/bash
# \
exec tclsh "$0" "$@"

#
# runs all tests and produces an html report
#

append tests "[glob tests/*.tcl]"
append tests " [glob tests/commands/*.tcl]"
append tests " [glob tests/bugs/*.tcl]"

proc htmlputs { txt } {
	global argv
	if { $argv == "-html" } {
		puts $txt
	}
}

htmlputs "<html><h1>Odamex regression test results</h1>"

set pass 0
set fail 0

foreach test $tests {

	# do not run self
	if { $test == $argv0 } {
		continue
	}
	
	# run test file
	set result [exec [info nameofexecutable] $test]

	# filter html tags
	if { $argv == "-html" } {
		set result [regsub -all < $result {\&lt;}]
		set result [regsub -all > $result {\&gt;}]
	}
	
	# html output
	htmlputs "<h2>$test</h2>"
	foreach line [split $result \n] {
		if { [regexp .*FAIL.* $line] } {
			htmlputs "<font color=red>"
			incr fail
		} elseif { [regexp .*PASS.* $line] } {
			htmlputs "<font color=green>"
			incr pass
		} else {
			htmlputs "<font color=black>"
		}
		puts $line
		htmlputs "</font><br />"
	}
}

puts "*** $pass passed, $fail failed ***"

htmlputs "</html>"
