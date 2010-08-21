#!/bin/bash
#
# Odamex demo regression stepped test
#
# Copyright (C) 2006-2010 by The Odamex Team.
# Author: Michael Wood
#
# Derived from stepped.sh
# Original Author: Denis Lukianov
#
# This script starts 2 clients and plays back the
# specified demo. The two clients are stepped
# frame by frame, with delays and checks in-between.
#
# The required parameters in order are:
#   Client 1 Executable
#   Client 2 Executable
#   IWad Containing Directory
#   IWAD
#   Demo Name (ex. demo1)
#
# An optional parameter allows a pause to occur
# after a given number of frames so that a debugger
# can be attached. The idea is to have a reproducible
# and visual model for debugging network events.
#

cln1=client1.control
cln2=client2.control
clno1=client1.out
clno2=client2.out
clne1=client1.err
clne2=client2.err
sleep="sleep 0.05"
ctrl_c=0

# Test for the required arguments
if [ "$5" == "" ]; then exit; fi;

# these are not the only demo parameters
# (see commands sent to server)
client1=$1
client2=$2
waddir=$3
iwad=$4
demoname=$5

# graceful exit on ctrl+c
trap 'ctrl_c=1' INT

# create control files
rm $cln1
rm $cln2
echo > $cln1
echo > $cln2
sleep 1

# launch client 1
echo "starting client 1"
tail -f -s 0.001 $cln1 | $client1 -waddir $waddir -iwad $iwad -nomouse -nosound -stepmode > $clno1 2> $clne1 &
echo "step" >> $cln1;
sleep 2
echo "set print_stdout 1" >> $cln1;

# launch client 2
echo "starting client 2"
tail -f -s 0.001 $cln2 | $client2 -waddir $waddir -iwad $iwad -nomouse -nosound -stepmode > $clno2 2> $clne2 &
echo "step" >> $cln2;
sleep 2
echo "set print_stdout 1" >> $cln2;

sleep 1

echo "playdemo $demoname" >> $cln1;
echo "playdemo $demoname" >> $cln2;
sleep 1
echo "step" >> $cln1;
echo "step" >> $cln2;
sleep 2

# start log mixer
echo "starting log mixer"
tail -f -s 0.001 -n 1 $clno2 $clne2 $clno1 $clne1 > mixed.out &

for i in `seq 1 1000`; do
 # compare last line of output
 l1=`tail -n 1 $clno1`
 l2=`tail -n 1 $clno2`
 if echo $l1 | grep -v "$l2" > tmp.out; then echo "desync detected ($l1|$l2)"; ctrl_c=1; fi;
 if echo $l1 | grep -v "$l2" > tmp.out;
 then
  echo "desync detected at step $i"
  echo "$clno1: $l1"
  echo "$clno2: $l2"
  ctrl_c=1;
 fi;
 # next step
 echo step $i
 # step each process
 echo step >> $cln2;
 $sleep
 echo step >> $cln1;
 $sleep
 # optionally run to step number given as parameter
 # then pause until a key press
 # (to allow developer to attach a debugger)
 if [ "$6" != "" ]
 then
   if (($i==$6))
   then
    echo "paused, please attach debugger and press enter to continue";
    read x
  fi
 fi
 # terminate if any process failed
 jobs -p > jobs.out
 if cat jobs.out | wc -l | grep -v 3 >> jobs.out; then break; fi;
 if echo $ctrl_c | grep 1 >> jobs.out; then break; fi;
done

echo "exiting"
sleep 1

echo quit >> $cln1
echo quit >> $cln2

sleep 1

# kill processes
jobs -p | sed "s/\([0-9]*\)/kill -9 \1/" | bash

echo done
