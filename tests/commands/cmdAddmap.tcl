#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 # try a two-map list
 clear
 server "clearmaplist"
 expect $serverout {Map list is already empty.}
 server "addmap MAP20"
 server "addmap MAP10"

 # first switch
 clear
 wait 10
 server "forcenextmap"
 expect $serverout {DOOM 2: Hell on Earth}
 expect $serverout {--- MAP20: gotcha! ---}

 # second switch
 clear
 wait 10
 server "forcenextmap"
 expect $serverout {DOOM 2: Hell on Earth}
 expect $serverout {--- MAP10: refueling base ---}

 # third switch
 clear
 wait 10
 server "forcenextmap"
 expect $serverout {DOOM 2: Hell on Earth}
 expect $serverout {--- MAP20: gotcha! ---}

 # clear the list
 clear
 server "clearmaplist"
 expect $serverout {}

 # try to switch
 clear
 wait 10
 server "forcenextmap"
 expect $serverout {Map list is empty.}

}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
