#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 wait

 # wad reboot while a client is connected
 clear
 server "wad doom2.wad"
 wait 5

 expect $serverout {DOOM 2: Hell on Earth}
 expect $serverout {adding odamex.wad}
 gets $serverout
 gets $serverout
 expect $serverout {(2919 lumps)}
 expect $serverout {--- MAP01: entryway ---}
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $clientout {connecting to 127.0.0.1:10599} 0
 set nextline [gets $clientout]
 if { $nextline == "connecting to 127.0.0.1:10599"} {
  # takes two connection attempts on slow machines
  gets $clientout
 }
 expect $clientout {> Server: Unnamed} 0
 expect $clientout {> Map: MAP01} 0
 expect $clientout {> DOOM2.WAD} 0
 expect $clientout {25E1459CA71D321525F84628F45CA8CD} 0
 expect $clientout {> Server protocol version: 65} 0 
 expect $clientout {> Server Version 0.4.1} 0 
gets $clientout
 expect $clientout {challenging 127.0.0.1:10599} 0
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {MAP01: entryway} 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
