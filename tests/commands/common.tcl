#!/bin/sh
# \
exec tclsh "$0" "$@"

package provide common 1.0

set port 10599

proc start {} {
 global server client serverout clientout port servercon clientcon

 set server [open odasrv.con w]
 set servercon [open "|./odasrv -port $port -logfile odasrv.log -confile odasrv.con > tmp" w]
 wait
 set serverout [open odasrv.log r]

 server "deathmatch 1"
 server "hostname Unnamed"
 server "maxclients 2"
 server "maxplayers 2"
 server "timelimit 0"
 server "map 1"

 set client [open odamex.con w]
 set clientcon [open "|./odamex -port 10501 -connect localhost:$port -nosound -novideo -logfile odamex.log -confile odamex.con > tmp" w]
 set clientout [open odamex.log r]

 wait 5

 client "print_stdout 1"
 client "cl_name Player"
}

proc end {} {
 global server client serverout clientout servercon clientcon

 client quit
 server quit

 close $serverout
 close $servercon
 close $server

 close $clientout
 close $clientcon
 close $client
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

proc wait { {seconds 1} } {
 set milliseconds [expr int($seconds*1000)]
 global endwait
 after $milliseconds set endwait 1
 vwait endwait
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

