#!/bin/sh

if [ -z ${1} ]; then
	echo "You need to specfy the path to a EIA shared library as an argument."
	exit 1;
fi

# Run make first
##
make clean all

# Run EIA test program
##
LD_LIBRARY_PATH=../../lib:$LD_LIBRARY_PATH               \
DYLD_LIBRARY_PATH=../../lib:$DYLD_LIBRARY_PATH           \
./TestEia.x $1
