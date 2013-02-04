#!/bin/bash
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc testAlias { module } {
 global server client serverout clientout port

 if { $module == "server"} {
  set out $serverout
  set offset 1
 } else {
  set out $clientout
  set offset 0
 }

 # enter a couple of commands and check history
 clear
 $module {a}
 $module {echo hi}
 $module {c}
 $module {history}
 expect $out {Unknown command a} $offset
 expect $out {hi} $offset
 expect $out {Unknown command c} $offset
 expect $out {} $offset

}

proc main {} {
 testAlias server
 testAlias client
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end
