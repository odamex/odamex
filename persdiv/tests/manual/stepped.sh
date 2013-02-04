#!/bin/bash
#
# Odamex Client-Server demo regression test
#
# Copyright (C) 2006-2007 by The Odamex Team.
# Author: Denis Lukianov
#
# This script connects a client to a server
# in such a way that the client streams demo input
# to the server. What happens on the server can
# then be compared to what happens on a second client
# that is playing the same demo on its own. The
# ensemble of two clients and a server is stepped
# frame by frame, with delays and checks in-between.
#
# An optional parameter allows a pause to occur
# after a given number of frames so that a debugger
# can be attached. The idea is to have a reproducible
# and visual model for debugging network events.
#

svr=server.control
cln=client.control
cln2=offline.control
svro=server.out
clno=client.out
clno2=offline.out
svre=server.err
clne=client.err
clne2=offline.err
sleep="sleep 0.05"
ctrl_c=0

# these are not the only demo parameters
# (see commands sent to server)
demoname=demo1 # up to 
map=map11
demoname=demo2 # up to 625
map=map05
#demoname=demo3
#map=map26

# graceful exit on ctrl+c
trap 'ctrl_c=1' INT

# create control files
rm $svr
rm $cln
rm $cln2
echo > $svr
echo > $cln
echo > $cln2
sleep 1

# launch server
echo "starting server"
tail -f -s 0.001 $svr | ./odasrv -stepmode > $svro 2> $svre &
sleep 2

echo sv_gametype	0   >> $svr
sleep 0.1
echo sv_nomonsters 0   >> $svr
sleep 0.1
echo sv_skill      4   >> $svr
sleep 0.1
echo sv_maxplayers 1   >> $svr
sleep 0.1
echo sv_speedhackfix 1 >> $svr
sleep 0.1

# launch client
echo "starting client"
tail -f -s 0.001 $cln | ./odamex -nomouse -nosound -stepmode -connect localhost > $clno 2> $clne &
sleep 2
echo "set print_stdout 1" >> $cln;

# launch offline client
echo "starting offline client"
tail -f -s 0.001 $cln2 | ./odamex -nomouse -nosound -stepmode > $clno2 2> $clne2 &
echo "step" >> $cln2;
sleep 2
echo "set print_stdout 1" >> $cln2;
sleep 1
echo "playdemo $demoname" >> $cln2;
sleep 1
echo "step" >> $cln2;
sleep 1

# wait for client to connect to the server
echo "client connecting to server"
for i in 1 2 3 4 5; do
 echo step >> $cln;
 sleep 1
 echo step >> $svr;
 sleep 1
done;

echo "starting demo"
echo map $map >> $svr
$sleep
echo step >> $svr;
$sleep
echo step >> $cln;
$sleep

echo streamdemo $demoname >> $cln
$sleep
echo step >> $cln;
$sleep

# start log mixer
echo "starting log mixer"
tail -f -s 0.001 -n 1 $clno2 $clne2 $svro $svre $clne > mixed.out &

for i in `seq 1 1000`; do
 # compare last line of output
 l3=`tail -n 1 $clno2`
 l2=`tail -n 1 $clno`
 l1=`tail -n 1 $svro | sed "s/\[.*\] //"`
# if echo $l1 | grep -v "$l2" > tmp.out; then echo "desync detected ($l1|$l3)"; ctrl_c=1; fi;
 if echo $l1 | grep -v "$l3" > tmp.out;
 then
  echo "desync detected at step $i"
  echo "$svro: $l1"
  echo "$clno2: $l3"
  ctrl_c=1;
 fi;
 # next step
 echo step $i
 # step each process
 echo step >> $cln2;
 $sleep
 echo step >> $cln;
 $sleep
 echo step >> $svr;
 $sleep
 # optionally run to step number given as parameter
 # then pause until a key press
 # (to allow developer to attach a debugger)
 if [ "$1" != "" ]
 then
   if (($i==$1))
   then
    echo "paused, please attach debugger and press enter to continue";
    read x
  fi
 fi
 # terminate if any process failed
 jobs -p > jobs.out
 if cat jobs.out | wc -l | grep -v 4 >> jobs.out; then break; fi;
 if echo $ctrl_c | grep 1 >> jobs.out; then break; fi;
done

echo "exiting"
sleep 1

echo quit >> $svr
echo quit >> $cln
echo quit >> $cln2

sleep 1

# kill processes
jobs -p | sed "s/\([0-9]*\)/kill -9 \1/" | bash

echo done
