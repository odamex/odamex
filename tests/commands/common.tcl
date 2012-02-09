#!/bin/sh
# \
exec tclsh "$0" "$@"

package provide common 1.0

set port 10599

proc startServer {} {
 global server serverout port servercon

 set server [open odasrv.con w]
 set servercon [open "|./odasrv -port $port +logfile odasrv.log -confile odasrv.con > tmp" w]
 wait
 set serverout [open odasrv.log r]

 server "sv_usemasters 0"
 server "sv_gametype 1"
 server "sv_hostname Unnamed"
 server "sv_email \"\""
 server "sv_maxclients 2"
 server "sv_maxplayers 2"
 server "sv_timelimit 0"
 server "sv_emptyreset 0"
 server "map 1"
}

proc startClient { {serverPort none} } {
 global client clientout clientcon

 set client [open odamex.con w]

 if { $serverPort != "none" } {
  set clientcon [open "|./odamex -port 10501 -connect localhost:$serverPort -nosound -novideo +logfile odamex.log -confile odamex.con > tmp" w]
 } else {
  set clientcon [open "|./odamex -port 10501 -nosound -novideo +logfile odamex.log -confile odamex.con > tmp" w]
 }
 set clientout [open odamex.log r]

 wait 5

 client "print_stdout 1"
 client "cl_name Player"
}

proc start {} {
 global server client serverout clientout port servercon clientcon

 startServer
 startClient $port
}

proc end {} {
 global server client serverout clientout servercon clientcon

 if { [info exists client] } {
  client quit
 }
 if { [info exists server] } {
  server quit
 }

 if { [info exists server] } {
  close $serverout
  close $servercon
  close $server
  unset server
 }

 if { [info exists client] } {
  close $clientout
  close $clientcon
  close $client
  unset client
 }
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
 global serverout clientout server client
 if { [info exists server] } {
  gets $serverout
  while { ![eof $serverout] } { gets $serverout }
 }
 if { [info exists client] } {
  gets $clientout
  while { ![eof $clientout] } { gets $clientout }
 }
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

