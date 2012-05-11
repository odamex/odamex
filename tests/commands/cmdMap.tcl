#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 # set defaults
 server "sv_globalspectatorchat 1"
 server "map 1"
 client "print_stdout 1"
 client "cl_name Player"

 wait

 # change map on server
 clear
 server "map 1"
 expect $serverout {--- MAP01: entryway ---}
 expect $clientout {}
 expect $clientout {}
 expect $clientout {MAP01: entryway} 0

 # change map on server
 clear
 server "map 02"
 expect $serverout {--- MAP02: underhalls ---}
 expect $clientout {}
 expect $clientout {}
 expect $clientout {MAP02: underhalls} 0

 # change map on server
 clear
 server "map map03"
 expect $serverout {--- map03: the gantlet ---}
 expect $clientout {}
 expect $clientout {}
 expect $clientout {map03: the gantlet} 0

 # change map on server
 clear
 server "map MAP04"
 expect $serverout {--- MAP04: the focus ---}
 expect $clientout {}
 expect $clientout {}
 expect $clientout {MAP04: the focus} 0

 # change map on client
 clear
 client "map MAP05"
 expect $serverout {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)}
 expect $clientout {}
 expect $clientout {}
 expect $clientout {MAP05: the waste tunnels} 0

 # change map on client
 clear
 client "map MAP06"
 expect $clientout {}
 expect $clientout {}
 expect $clientout {MAP06: the crusher} 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
