#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 wait

 # kick wrong player
 clear
 server "kick 99"
 expect $serverout {bad client number: 0}
 expect $clientout {} 0

 # kick the player
 clear
 server "kick 1"

 expect $serverout {Player was kicked from the server!}
 expect $serverout {}
 expect $clientout {Player was kicked from the server!} 0
 expect $clientout {} 0
 expect $clientout {Server disconnected} 0
 expect $clientout {} 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
