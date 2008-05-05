#!/bin/sh
# \
exec tclsh "$0" "$@"

package provide common 1.0

set port 10599

proc start {} {
 global server client serverout clientout port
 set server [open "|./odasrv -port $port > odasrv.log" w]
 set serverout [open odasrv.log r]

 server "maxclients 2"
 server "maxplayers 2"
 server "timelimit 0"
 server "map 1"

 set client [open "|./odamex -port 10501 -connect localhost:$port -nosound -novideo > odamex.log" w]
 set clientout [open odamex.log r]

 wait 5

 client "print_stdout 1"
 client "cl_name Player"
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
}

proc client { cmd } {
 global client clientout
 puts $client $cmd
 flush $client

 wait
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

