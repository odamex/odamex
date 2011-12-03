#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc deathmatch {} {
 global server client serverout clientout port

 server "sv_gametype 1"

 # 7 second sv_timelimit
 clear
 server "map 1"
 server "sv_timelimit 0.125"
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

 server "sv_gametype 0"

 # 7 second sv_timelimit
 clear
 server "map 1"
 server "sv_timelimit 0.125"
 server "map 2"
 clear

 # not yet
 wait 5
 expect $serverout ""
 expect $serverout ""

 # wait our time
 wait 5
 expect $serverout ""
 expect $serverout "COOPERATIVE"
 expect $serverout "-----------------------------------------------------------" 
}

proc teamplay {} {
 global server client serverout clientout port

 server "sv_gametype 2"

 # 7 second sv_timelimit
 clear
 server "map 1"
 server "sv_timelimit 0.125"
 server "map 2"
 clear

 # not yet
 wait 5
 expect $serverout ""
 expect $serverout ""

 # wait our time
 wait 5
 expect $serverout "Time limit hit. Game is a draw!"
 expect $clientout "Time limit hit. Game is a draw!" 0
}


proc main {} {
 deathmatch
 coop
 teamplay
 # reset
 server "sv_timelimit 0"
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
