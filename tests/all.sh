#!/bin/bash
for i in tests/*.tcl; do echo $i; $i; done;
for i in tests/commands/*.tcl; do echo $i; $i; done;
