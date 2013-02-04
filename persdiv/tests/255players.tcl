#!/bin/sh
# \
exec tclsh "$0" "$@"

set port        10599
set numplayers  3; # 255

proc start {} {
 global server client serverout clientout port numplayers
 set server [open "|./odasrv -port $port +logfile odasrv.log > tmp" w]
 wait
 set serverout [open odasrv.log r]

 server "sv_gametype 1"
 server "sv_maxclients $numplayers"
 server "sv_maxplayers $numplayers"
 server "sv_timelimit 0"
 server "map 1"

 # clear server only
 while { ![eof $serverout] } { gets $serverout }

 array set client ""
 for {set i 0} {$i < $numplayers} {incr i} {
  set client($i) [open "|./odamex -port [expr 10401+$i] -connect localhost:$port -nosound -novideo +logfile odamex$i.log > tmp" w]
  if { $client($i) == "" } {
   puts "FAIL: could not start client $i"
  } else {
   puts -nonewline .
   flush stdout
  }
  wait 2
 }
 puts ""

 wait 5
}

proc check {} {
 global server client serverout clientout port numplayers
 set ok 0
 for {set i 0} {$i < $numplayers} {incr i} {
  gets $serverout
  if { [lrange [gets $serverout] 1 end] == "Player has connected." } { incr ok }
 }
 for {set i 0} {$i < $numplayers} {incr i} {
  if { [lrange [gets $serverout] 1 end] == {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)} } { incr ok }
 }
 if { [expr $numplayers*2] == $ok } {
  puts "PASS ($numplayers players)"
 } else {
  puts "FAIL ([expr $ok/2]/$numplayers)"
 }
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

 check

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

proc main {} {
 start
}

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end

