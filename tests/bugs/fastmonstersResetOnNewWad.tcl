#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global client clientout

 wait

 # run av.wad demo1
 client "wad doom2.wad av.wad"
 wait 4
 clear
 client "playdemo demo1"
 wait 10

 gets $clientout
 gets $clientout
 gets $clientout
 expect $clientout {MAP07: dead simple} 0
 gets $clientout
 expect $clientout {You got the super shotgun!} 0
 
 # run doom1.wad demo1
 client "wad doom1.wad"
 wait 4
 clear
 client "playdemo demo1"
 wait 10

 gets $clientout
 gets $clientout
 gets $clientout
 expect $clientout {E1M5: Phobos Lab} 0
 gets $clientout
 expect $clientout {Picked up the armor.} 0
 expect $clientout {You got the shotgun!} 0
 # the following lines fail on r1264
 expect $clientout {Picked up an armor bonus.} 0
 expect $clientout {Picked up an armor bonus.} 0
 expect $clientout {Picked up an armor bonus.} 0
}

startClient

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
