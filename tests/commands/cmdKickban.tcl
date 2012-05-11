#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 wait

 # kickban wrong player
 clear
 server "kickban 99"
 expect $serverout {bad client number: 99}
 expect $clientout {} 0

 # kickban wrong player with reason
 clear
 server "kickban 99 be gone"
 expect $serverout {bad client number: 99}
 expect $clientout {} 0

 # kickban the player with reason
 clear
 server "kickban 1 be gone"

 expect $serverout {Player was kickbanned from the server! (Reason: be gone)}
 expect $serverout {Ban on 127.0.0.1 added.}
 expect $serverout {}
 expect $clientout {Player was kickbanned from the server! (Reason: be gone)} 0
 expect $clientout {} 0
 expect $clientout {Server disconnected} 0
 expect $clientout {} 0

 # should fail to reconnect
 clear
 client "reconnect"
 expect $serverout {127.0.0.1:10501 is trying to connect...}
 expect $serverout {127.0.0.1:10501 is banned and unable to join! (reason: be gone)}
 expect $serverout {}
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
