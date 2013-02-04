#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout port

 # change player name
 clear
 wait 6
 client {cl_name Fred}
 expect $serverout "Player changed his name to Fred."

 # long name
 clear
 wait 6
 client {cl_name OneVeryVeryVeryLongName}
 expect $serverout "Fred changed his name to OneVeryVeryVery."

 # blank name
 clear
 wait 6
 client {set cl_name ""}
 expect $serverout "OneVeryVeryVery changed his name to ."
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
