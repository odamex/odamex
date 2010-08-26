#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global client clientout

 wait

 clear
 # use the wad command without an iwad
 client "wad av.wad"
 wait 4
 
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {DOOM 2: Hell on Earth} 0
 expect $clientout {} 0
 
 clear

 # try to run av.wad demo1
 client "playdemo demo1"
 wait 4

 expect $clientout {Playing demo demo1} 0
 expect $clientout {} 0
}

startClient

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
