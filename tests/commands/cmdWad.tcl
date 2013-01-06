#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 wait

 # wad reboot while a client is connected
 clear
 server "wad doom.wad"
 wait 5

 expect $serverout {The Ultimate DOOM}
 expect $serverout {adding odamex.wad}
 gets $serverout
 gets $serverout
 expect $serverout {(2306 lumps)}
 expect $serverout {--- E1M1: Hangar ---}
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $clientout {connecting to 127.0.0.1:10599} 0
 set nextline [gets $clientout]
 if { $nextline == "connecting to 127.0.0.1:10599"} {
  # takes two connection attempts on slow machines
  gets $clientout
 }
 expect $clientout {> Server: Unnamed} 0
 expect $clientout {> Map: E1M1} 0
 expect $clientout {> DOOM.WAD} 0
 expect $clientout {C4FE9FD920207691A9F493668E0A2083} 0
 expect $clientout {> Server protocol version: 65} 0 

 set expectedVersionString "> Server Version "
 set nextline [gets $clientout]
 if { [string range $nextline 0 16] != $expectedVersionString } {
  puts "FAIL ($expectedVersionString|[string range $nextline 0 16])"
 } else {
  puts "PASS $nextline"
 }
 gets $clientout
 gets $clientout
 expect $clientout {=================================} 0
 expect $clientout {The Ultimate DOOM} 0
 expect $clientout {} 0
 expect $clientout {adding odamex.wad} 0
 gets $clientout
 gets $clientout
 expect $clientout {(2306 lumps)} 0
 expect $clientout {challenging 127.0.0.1:10599} 0
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {E1M1: Hangar} 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
