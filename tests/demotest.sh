#!/bin/bash
#
# runs ./odamex -nosound -novideo +demotest DEMONAME[.LMP] -file WADFILE.WAD
# against list of demos in stdin
#
# assumes stdin input format:
# WADFILE.WAD DEMONAME[.LMP] numbera numberb numberc numberd
#
# produces output format like:
# DOOM2.WAD DEMO1 [PASS]
# DOOM2.WAD TEST.LMP [FAIL]
#

awk '{print "echo -n \"" $1 " " $2 " "$3 " " $4 " " $5 " " $6 " \"; ./odamex -nosound -novideo +demotest " $2 " -file " $1}' | \
bash | \
awk '{print $1 " " $2 " " $3 $4 $5 $6 " "$7 $8 $9 $10}' | \
awk '{if($3 == $4) print $1 " " $2 " [PASS]"; else print $1 " " $2 " [FAIL] " $7 " " $8 " " $9 " " $10}'
