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

 # try to load a map (fails)
 client "map1"
 wait 4

 expect $clientout {Unknown command map1} 0
 expect $clientout {} 0

 # try to load a nonexistant wad (fails but does not crash)
 client "wad 404"
 wait 4

 expect $clientout {I_InitMusic: Music playback disabled.} 0
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {DOOM 2: Hell on Earth} 0
 gets $clientout
 expect $clientout {adding odamex.wad} 0
}

startClient

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
