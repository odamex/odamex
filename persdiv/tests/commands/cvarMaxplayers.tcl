#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout port

 # set defaults
 server "sv_globalspectatorchat 1"
 server "map 1"
 client "print_stdout 1"
 client "cl_name Player"

 wait

 # reset sv_maxclients and sv_maxplayers
 clear
 server "sv_maxclients 2"
 server "sv_maxplayers 2"
 server "map 1"
 client "disconnect"
 client "reconnect"
 wait 3

 # kick client off the server
 clear
 server "sv_maxclients 0"
 expect $serverout {sv_maxclients will be changed for next game.}
 server "sv_maxplayers 0"
 expect $serverout {sv_maxplayers will be changed for next game.}
 server "map 1"
 expect $serverout {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)}
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {MAP01: entryway} 0
 expect $clientout {} 0
 expect $clientout {Client limit reduced. Please try connecting again later.} 0
 expect $clientout {} 0
 expect $clientout {Server disconnected} 0

 # try to connect
 clear
 client "disconnect"
 client "reconnect"
 expect $clientout "connecting to 127.0.0.1:$port" 0
 wait 3
 for {set i 0} {$i < 11} {incr i} { gets $clientout }
 expect $clientout {Server is full} 0

 # change sv_maxplayers (non-latched)
 clear
 server "sv_maxplayers 2"
 expect $serverout {sv_maxplayers will be changed for next game.}
 server "map 1"

 # try to connect
 clear
 client "disconnect"
 client "reconnect"
 expect $clientout "connecting to 127.0.0.1:$port" 0
 wait 3
 for {set i 0} {$i < 11} {incr i} { gets $clientout }
 expect $clientout {Server is full} 0

 # change sv_maxplayers and sv_maxclients
 clear
 server "sv_maxclients 2"
 expect $serverout {sv_maxclients will be changed for next game.}
 server "sv_maxplayers 0"
 expect $serverout {sv_maxplayers will be changed for next game.}
 server "map 1"
 clear

 # try to connect
 clear
 client "disconnect"
 client "reconnect"
 expect $clientout "connecting to 127.0.0.1:$port" 0
 wait 3
 for {set i 0} {$i < 11} {incr i} { gets $clientout }
 expect $clientout {} 0
 expect $clientout {=================================} 0
 expect $clientout {MAP01: entryway} 0

 # try to join with sv_maxplayers 0
 clear
 client "+use"
 wait
 client "-use"
 expect $serverout {}

 # change sv_maxplayers
 clear
 server "sv_maxplayers 2"
 expect $serverout {sv_maxplayers will be changed for next game.}
 server "map 1"

 # try to join
 clear
 client "+use"
 wait
 client "-use"
 expect $serverout {Player joined the game.}

 # change sv_maxplayers and player should become a spectator
 clear
 server "sv_maxplayers 0"
 expect $serverout {sv_maxplayers will be changed for next game.}
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
