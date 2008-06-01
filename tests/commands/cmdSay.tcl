#!/bin/sh
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 # set defaults
 server "globalspectatorchat 1"
 server "teamplay 0"
 server "map 1"
 client "print_stdout 1"
 client "cl_name Player"

 wait

 # talk as spectator
 clear
 test "say hello" "Player: hello"
 test "say_team hello" "<Player to SPECTATORS> hello"

 # disable global chat
 server "globalspectatorchat 0"
 clear
 test "say hello" "<Player to SPECTATORS> hello"
 test "say_team hello" "<Player to SPECTATORS> hello"

 # join the game
 clear
 test "join" "Player joined the game."
 test "say hello" "Player: hello"
 test "say_team hello" ""

 # test teamplay
 server "teamplay 1; map 1"
 clear
 test "join" "Player joined the game on the BLUE team."
 test "say hello" "Player: hello"
 test "say_team hello" "<Player to TEAM> hello"

 # test server console
 clear
 server "say hello"
 expect $clientout {[console]: hello} 0 
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end

