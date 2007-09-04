#!/bin/bash
#
# Runs a server and a client, checks for a successful game join
#

./odasrv | awk '{}' &

./odamex -novideo -nosound -connect localhost +set print_stdout 1 > tmp &

sleep 10

kill -9 %1 %2

cat tmp \
| grep "entered the game" \
| wc -l \
| awk '{if($1 == "1") print "PASS"; else print "FAIL"}'


