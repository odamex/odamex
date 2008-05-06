#!/bin/sh
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout port

 wait

 # kill server
 clear
 puts $server "rquit"
 close $server
 close $serverout
 wait

 # check that client is reconnecting
 expect $clientout {connecting to 127.0.0.1:10599} 0

 # restart server
 set server [open "|./odasrv -port $port > odasrv.log" w]
 wait
 set serverout [open odasrv.log r]
 clear

 # check that client is reconnected
 wait 5
 for {set i 0} {$i < 8} {incr i} { gets $clientout }
 expect $clientout {challenging 127.0.0.1:10599} 0
 expect $clientout {Odamex Deathmatch Server} 0
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end

