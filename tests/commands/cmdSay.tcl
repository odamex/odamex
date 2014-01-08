#!/bin/sh
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 # set defaults
 server "sv_globalspectatorchat 1"
 server "map 1"
 client "print_stdout 1"
 client "cl_name Player"

 wait

 # talk as spectator
 clear
 test "say hello" "Player: hello"
 expect $clientout {Player: hello} 0
 wait 1
 test "say_team hello" "<Player to SPECTATORS> hello"
 
 # test me 
 wait 1
 clear
 test "say /me action" "* Player action"
 wait 1
 test "say_team /me action" "<SPECTATORS> * Player action"
 wait 1

 # disable global chat
 server "sv_globalspectatorchat 0"
 clear
 wait 1
 test "say hello" "<Player to SPECTATORS> hello"
 wait 1
 test "say_team hello" "<Player to SPECTATORS> hello"

 wait 1
 # join the game
 clear
 test "join" "Player joined the game."
 clear
 test "say hello" "Player: hello"
 expect $clientout {Player: hello} 0
 wait 1
 test "say_team hello" ""

 wait 1
 # test teamplay
 client "cl_team blue"
 server "sv_gametype 2; map 1"
 clear
 test "join" "Player joined the game on the BLUE team."
 clear
 test "say hello" "Player: hello"
 expect $clientout {Player: hello} 0
 wait 1
 test "say_team hello" "<Player to TEAM> hello"
 
 # test /me
 wait 1
 clear
 test "say_team /me action" "<TEAM> * Player action"
 wait 1

 wait 1
 # test flood protection
 clear
 test "say hello1" "Player: hello1"
 test "say hello2" ""
 wait 1
 test "say hello3" "Player: hello3"

 wait 1
 # test classic csdoom string parsing mistake
 clear
 test "say %s%s%i%s\"\'" "Player: %s%s%i%s"
 wait 1
 test "say veryveryveryverylonglongstringstringstringveryveryveryverylonglongstringstringstringveryveryveryverylonglongstringstringstringveryveryveryverylonglongstringstringstring" "Player: veryveryveryverylonglongstringstringstringveryveryveryverylonglongstringstringstringveryveryveryverylonglongstringstringstringve"

 # test server console
 clear
 server "say hello"
 expect $clientout {[console]: hello} 0 
 server "say %s"
 expect $clientout {[console]: %s} 0 

}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end

