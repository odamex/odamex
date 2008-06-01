#!/bin/bash
# \
exec tclsh "$0" "$@"

#
# runs all tests and produces an html report
#

append tests "[glob tests/*.tcl]"
append tests " [glob tests/commands/*.tcl]"

puts "<html><h1>Odamex regression test results</h1>"

set pass 0
set fail 0

foreach test $tests {

	# do not run self
	if { $test == $argv0 } {
		continue
	}
	
	# run test file
	set result [exec $test]
	
	# html output
	puts "<h2>$test</h2>"
	foreach line [split $result \n] {
		if { [regexp .*FAIL.* $line] } {
			puts "<font color=red>"
			incr fail
		} elseif { [regexp .*PASS.* $line] } {
			puts "<font color=green>"
			incr pass
		} else {
			puts "<font color=black>"
		}
		puts $line
		puts "</font><br />"
	}
}

puts "$pass passed, $fail failed"

puts "</html>"
