#!/bin/sh
# \
exec tclsh "$0" "$@"

set port        10599
set numplayers  25; # 255

proc start {} {
 global server client serverout clientout port numplayers
 set server [open "|./odasrv -port $port > odasrv.log" w]
 wait
 set serverout [open odasrv.log r]

 server "maxclients $numplayers"
 server "maxplayers $numplayers"
 server "timelimit 0"
 server "map 1"

 array set client ""
 for {set i 0} {$i < $numplayers} {incr i} {
  set client($i) [open "|./odamex -port [expr 10401+$i] -connect localhost:$port -nosound -novideo > odamex$i.log" w]
  if { $client($i) == "" } {
   puts "FAIL: could not start client $i"
  } else {
   puts -nonewline .
   flush stdout
  }
  wait
 }
 puts ""

 wait 10
}

proc end {} {
 global server client serverout clientout numplayers

 for {set i 0} {$i < $numplayers} {incr i} {
  puts $client($i) quit
 }

 wait

 for {set i 0} {$i < $numplayers} {incr i} {
  close $client($i)
 }

 wait 5

 server quit

 close $server
 close $serverout
}

proc server { cmd } {
 global server serverout
 puts $server $cmd
 flush $server

 wait
}

proc wait { {n 1} } {
 exec sleep $n
}

proc main {} {
 start
 end
}


set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

