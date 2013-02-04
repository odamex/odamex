#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 wait

 # kick wrong player
 clear
 server "kick 99"
 expect $serverout {kick: could not find client 99.}
 expect $clientout {} 0

 # kick wrong player with reason
 clear
 server "kick 99 be gone"
 expect $serverout {kick: could not find client 99.}
 expect $clientout {} 0

 # kick the player
 clear
 server "kick 1"

 expect $serverout {Player was kicked from the server!}
 expect $serverout {}
 expect $clientout {Player was kicked from the server!} 0
 expect $clientout {} 0
 expect $clientout {Server disconnected} 0
 expect $clientout {} 0

 # reconnect
 client "reconnect"

 # kick the player with reason
 clear
 server "kick 1 be gone"

 expect $serverout {Player was kicked from the server! (Reason: be gone)}
 expect $serverout {}
 expect $clientout {Player was kicked from the server! (Reason: be gone)} 0
 expect $clientout {} 0
 expect $clientout {Server disconnected} 0
 expect $clientout {} 0

}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
