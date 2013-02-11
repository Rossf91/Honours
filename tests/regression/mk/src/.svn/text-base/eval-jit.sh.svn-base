#!/bin/bash
# Simple test script - <igor.boehm@ed.ac.uk>

# ------------------------------------------------------------------------------
# Parameters
# ------------------------------------------------------------------------------
SIM=$1
BMARK=$2
OUTPUT=$3
EXPECTED=$4

# ------------------------------------------------------------------------------
# Check Parameters
# ------------------------------------------------------------------------------
if [ ! -x $SIM ]; then
	echo "ERROR: Can not execute simulator '$SIM'."
	exit -1
fi

if [ ! -e $BMARK ]; then
	echo "ERROR: Benchmark '$BMARK' does not exist."
	exit -1
fi

if [ ! -n "$OUTPUT" ]; then
	echo "ERROR: No output file specified."
	exit -1
fi

if [ ! -n "$EXPECTED" ]; then
	echo "ERROR: No expected value specified."
	exit -1
fi

# ------------------------------------------------------------------------------
# Configurations
# ------------------------------------------------------------------------------

COMPILERS="gcc"
LTUMODES="bb scc cfg page"
JITOPTS="-O0 -O1 -O2 -O3 -Os"
THRESHOLDS="1"

# ------------------------------------------------------------------------------

HEADER="BENCHMARK,T-SIM,T-ANALYSIS,T-COMP,T-TOTAL,MIPS,CONF,TEST"

if [ ! -e $OUTPUT ]; then
	echo $HEADER >> $OUTPUT
fi

# ------------------------------------------------------------------------------
# Main simulation loop
# ------------------------------------------------------------------------------
BBMARK=`basename $BMARK`

for comp in $COMPILERS; do
	for ltumode in $LTUMODES; do
		for jitopt in $JITOPTS; do
			for thresh in $THRESHOLDS; do
				# create temporary directory and file for JIT code and log
				TMPDIR=`mktemp -d /tmp/cfg.arcsim.XXXXXXXXXX`
				TMPOUT=`mktemp /tmp/cfg.arcsim.XXXXXXXXXX`

				# execute simulator
				${SIM}                         \
					--verbose                    \
					--debug                      \
					--keep                       \
					--fast                       \
					--fast-cc=$comp              \
					--fast-trans-mode=$ltumode   \
					--fast-cc-opt=$jitopt        \
					--fast-thresh=$thresh        \
					--fast-tmp-dir=$TMPDIR/jit   \
					-e $BMARK &> $TMPOUT

				# extract executed instructions
				ACTUAL=`awk '/^ Total instructions/ {print $3}' $TMPOUT`

				# perform sanity check
				if [ "X$EXPECTED" != "X$ACTUAL" ]; then
					TEST="FAILED[EXP:$EXPECTED|ACT:$ACTUAL]"
				else
					TEST="PASSED"
				fi

				# extract simulation speed
				MIPS=`awk '/^Simulation rate/ {print $4}' $TMPOUT`
				# extract timing
				TIME=`awk '/^ Time:/ { printf("%s,%s,%s,%s",$2,$3,$5,$6)}' $TMPOUT`
				# emit information into specified output file
				RESULT="$BBMARK,$TIME,$MIPS,$comp|$ltumode|$jitopt|$thresh,$TEST"
				echo $RESULT >> $OUTPUT

				echo "=== $RESULT - [$HEADER]"
				# remove temporary stuff
				rm -rf $TMPDIR $TMPOUT
			done
		done
	done
done

exit 0
