#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout port

 # set defaults
 server "globalspectatorchat 1"
 server "map 1"
 client "print_stdout 1"
 client "cl_name Player"

 wait

 # reset maxclients and maxplayers
 clear
 server "maxclients 2"
 server "maxplayers 2"
 server "map 1"
 client "disconnect"
 client "reconnect"
 wait 3

 # kick client off the server
 clear
 server "maxclients 0"
 expect $serverout {maxclients will be changed for next game.}
 server "maxplayers 0"
 expect $serverout {maxplayers will be changed for next game.}
 server "map 1"
 expect $serverout {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)}

 # try to connect
 clear
 client "disconnect"
 client "reconnect"
 expect $clientout "connecting to 127.0.0.1:$port" 0
 wait 3
 for {set i 0} {$i < 9} {incr i} { gets $clientout }
 expect $clientout {Server is full} 0

 # change maxplayers (non-latched)
 clear
 server "maxplayers 2"
 expect $serverout {maxplayers will be changed for next game.}
 server "map 1"

 # try to connect
 clear
 client "disconnect"
 client "reconnect"
 expect $clientout "connecting to 127.0.0.1:$port" 0
 wait 3
 for {set i 0} {$i < 9} {incr i} { gets $clientout }
 expect $clientout {Server is full} 0

 # change maxplayers and maxclients
 clear
 server "maxclients 2"
 expect $serverout {maxclients will be changed for next game.}
 server "maxplayers 0"
 expect $serverout {maxplayers will be changed for next game.}
 server "map 1"
 clear

 # try to connect
 clear
 client "disconnect"
 client "reconnect"
 expect $clientout "connecting to 127.0.0.1:$port" 0
 wait 3
 for {set i 0} {$i < 9} {incr i} { gets $clientout }
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {MAP01: entryway} 0

 # try to join with maxplayers 0
 clear
 client "+use"
 wait
 client "-use"
 expect $serverout {}

 # change maxplayers
 clear
 server "maxplayers 2"
 expect $serverout {maxplayers will be changed for next game.}
 server "map 1"

 # try to join
 clear
 client "+use"
 wait
 client "-use"
 expect $serverout {Player joined the game.}

 # change maxplayers and player should become a spectator
 clear
 server "maxplayers 0"
 expect $serverout {maxplayers will be changed for next game.}
 server "map 1"
 expect $serverout {Player became a spectator.}
 expect $serverout {--- MAP01: entryway ---}
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
