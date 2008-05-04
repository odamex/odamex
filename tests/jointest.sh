#!/bin/bash
#
# Runs a server and a client, checks for a successful game join
#

set port=10660

./odasrv -port $port | awk '{}' &

./odamex -novideo -nosound -connect localhost:$port +set print_stdout 1 > tmp &

sleep 10

kill -9 %1 %2

cat tmp \
| grep "has connected" \
| wc -l \
| awk '{if($1 == "1") print "PASS"; else print "FAIL"}'


