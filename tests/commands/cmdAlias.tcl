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

 # add an alias
 clear
 $module {alias cmdAlias "echo \"Hello, world!\""}
 expect $out {}
 $module {cmdAlias}
 expect $out {Hello, world!} $offset

 # remove the alias
 clear
 $module {alias cmdAlias}
 expect $out {}
 $module {cmdAlias}
 expect $out {Unknown command cmdAlias} $offset

 # add an alias to an alias
 clear
 $module {alias cmdAlias "echo \"Hello, world!\""}
 $module {alias cmdAlias2 "cmdAlias"}
 expect $out {}
 $module {cmdAlias2}
 expect $out {Hello, world!} $offset
 $module {alias cmdAlias}
 $module {alias cmdAlias2}
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
