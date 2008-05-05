#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout port

 # 7 second timelimit
 clear
 server "map 1"
 server "timelimit 0.125"
 server "map 2"
 clear

 # not yet
 wait 5
 expect $serverout ""
 expect $serverout ""

 # wait our time
 wait 5
 expect $serverout ""
 expect $serverout "--------------------------------------"
 expect $clientout "" 0
 expect $clientout "--------------------------------------" 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
