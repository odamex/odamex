#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc simple {} {
 global server client serverout clientout port

 # add ban
 clear
 server "addban 127.0.0.1"
 expect $serverout "Ban on 127.0.0.1 added."

 # try to reconnect
 client "disconnect"
 client "reconnect"
 wait 2
 for {set i 0} {$i < 12} {incr i} { gets $clientout }
 expect $clientout {} 0 
 expect $clientout {You are banned! (reason: none given)} 0
 expect $clientout {If you feel there has been an error, contact the server host. (No e-mail given)} 0
 expect $serverout {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)}
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $serverout {127.0.0.1:10501 is banned and unable to join! (reason: none given)}

 # remove ban
 clear
 server "delban 127.0.0.1"
 expect $serverout "1 Bans removed."

 # reconnect should have worked now
 clear
 client "disconnect"
 client "reconnect"
 wait 2
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $serverout {Player has connected.}
}

proc wildcard {} {
 global server client serverout clientout port

 # add ban
 clear
 server "addban 127.0.0.*"
 expect $serverout "Ban on 127.0.0.* added."

 # try to reconnect
 client "disconnect"
 client "reconnect"
 wait 2
 for {set i 0} {$i < 12} {incr i} { gets $clientout }
 expect $clientout {} 0
 expect $clientout {You are banned! (reason: none given)} 0
 expect $clientout {If you feel there has been an error, contact the server host. (No e-mail given)} 0
 expect $serverout {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)}
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $serverout {127.0.0.1:10501 is banned and unable to join! (reason: none given)}

 # remove ban
 clear
 server "delban 127.0.0.*"
 expect $serverout "1 Bans removed."

 # reconnect should have worked now
 clear
 client "disconnect"
 client "reconnect"
 wait 2
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $serverout {Player has connected.}
}

proc notme {} {
 global server client serverout clientout port

 # add ban
 clear
 server "addban 128.0.0.*"
 expect $serverout "Ban on 128.0.0.* added."

 # reconnect should have worked now
 clear
 client "disconnect"
 client "reconnect"
 wait 2
 expect $serverout {Player disconnected. (SPECTATOR, 0 FRAGS, 0 DEATHS)}
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $serverout {Player has connected.}
}

proc main {} {
 simple
 wildcard
 notme
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
