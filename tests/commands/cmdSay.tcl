#!/bin/sh
# \
exec tclsh "$0" "$@"

proc start {} {
 global server client serverout clientout
 set server [open "|./odasrv -port 10599 > odasrv.log" w]
 set serverout [open odasrv.log r]

 set client [open "|./odamex -connect localhost:10599 -nosound -novideo > odamex.log" w]
 set clientout [open odamex.log r]

 wait 5
}

proc end {} {
 global server client serverout clientout

 client quit
 server quit

 close $server
 close $serverout

 close $client
 close $clientout
}

proc server { cmd } {
 global server serverout
 puts $server $cmd
 flush $server

 wait

 while { ![eof $serverout] } { gets $serverout }
}

proc client { cmd } {
 global client clientout
 puts $client $cmd
 flush $client

 wait

 while { ![eof $clientout] } { gets $clientout }
}

proc clear {} {
 global serverout clientout
 gets $serverout
 gets $clientout
 while { ![eof $serverout] } { gets $serverout }
 while { ![eof $clientout] } { gets $clientout }
}

proc wait { {n 1} } {
 exec sleep $n
}

proc expect { stream expected {excludeTimestamp 1} } {
 # strip the timestamp
 set out [lrange [gets $stream] $excludeTimestamp end]
 set out [join $out " "]
 if { $expected == $out } {
  puts "PASS $expected"
 } else {
  puts "FAIL ($expected|$out)"
 }
}

proc test { cmd expect } {
 global server client serverout clientout

 client $cmd
 expect $serverout $expect
}

proc main {} {
 global server client serverout clientout

 # set defaults
 server "globalspectatorchat 1"
 server "teamplay 0"
 server "map 1"
 client "print_stdout 1"
 client "name Player"

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

