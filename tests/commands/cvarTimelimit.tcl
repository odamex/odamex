#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc deathmatch {} {
 global server client serverout clientout port

 server "deathmatch 1"

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
 expect $serverout "Time limit hit. Game won by Player!"
 expect $clientout "Time limit hit. Game won by Player!" 0
}

proc coop {} {
 global server client serverout clientout port

 server "deathmatch 0"

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


proc main {} {
 deathmatch
 coop
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
