#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 wait

 # playdemo on the client
 clear
 client "playdemo demo1"

 wait 35

 expect $clientout {Playing demo demo1} 0
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {MAP11: 'o' of destruction!} 0
 for {set i 0} {$i < 6} {incr i} {
  gets $clientout
 }
 expect $clientout {You got the shotgun!} 0
 expect $clientout {You got the chaingun!} 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
