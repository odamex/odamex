#!/bin/bash
for i in tests/*; do echo $i | grep -v $0 | grep "\.sh" | awk '{print "echo -n " $1 "\" \" ;" $1 " 2>>tmp"}' | bash; done;
