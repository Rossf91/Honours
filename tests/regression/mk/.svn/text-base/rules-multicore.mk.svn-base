#################################################################################
# mk/rules-multicore.mk
#
# JIT performance/statistics for multicore simulation.
#################################################################################

test-regression-splash:
	for t in $(MC_SPLASH_BENCHMARKS) ; do                                         \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		TESTLOG=`echo "$$TEST" | sed 's/\//X/g'`;                                   \
		pushd $(TOP)/tests/multicore/splash/$$TEST > /dev/null 2>&1 ;               \
			SIMLOGDIR=`mktemp -d /tmp/arcsim.XXXXXXXXXX`;                             \
				${SIM} ${SIMOPT} ${SIMARGS}                       \
				${SIMRUNARG} `cat runcmd` &> $$SIMLOGDIR/$$TESTLOG.log;                 \
				RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
				TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
				MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
				CYCLES=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;           \
			echo "=== TEST: [SPLASH] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLES [Cycles]"; \
			if test -d $$SIMLOGDIR ; then rm -rf $$SIMLOGDIR; fi; \
		popd > /dev/null 2>&1  ; \
	done



