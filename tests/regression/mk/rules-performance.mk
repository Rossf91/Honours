#################################################################################
# mk/rules-performance.mk
#
# JIT performance targets
#################################################################################

ITERATIONS=9

#--------------------------------------------------------------------------------
# @Target: test-performance
# @Description: Run regression test versions of benchmarks
#--------------------------------------------------------------------------------
test-performance: setup
	$(Echo) "== Running Performance Regression Test Suite"
	$(Verb) _EEMBC_TESTS="${EEMBC_LARGE_ITER}"       make _test-performance-eembc    | tee    ${LOG}/$@.log
	$(Verb) _BIOPERF_TESTS="${BIOPERF_DEFAULT_ITER}" make _test-performance-bioperf  | tee -a ${LOG}/$@.log
	$(Verb) _SPEC_TESTS="${SPEC_DEFAULT_ITER}"       make _test-performance-spec     | tee -a ${LOG}/$@.log

#--------------------------------------------------------------------------------
# @Target: test-performance-eembc
# @Description: Performance test EEMBC
#--------------------------------------------------------------------------------
_test-performance-eembc:
	$(Verb) for t in ${_EEMBC_TESTS} ; do                                         \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.XXXXXXXXXX`;                               \
		echo "=== RUNNING: 'eembc/$$TEST'";                                         \
		touch $$SIMLOGDIR/mips-list ;                                               \
		touch $$SIMLOGDIR/assoc-list ;                                              \
		for iter in {1..${ITERATIONS}} ; do                                         \
			${SIM}                                                                    \
					${SIMOPT}                                                             \
					--fast-tmp-dir=$$SIMLOGDIR/jit                                        \
					${SIMARGS}                                                            \
					${SIMRUNARG} $(TOP)/tests/eembc/large-iterations/$$TEST &> $$SIMLOGDIR/$$TEST.log; \
			RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;  \
			TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
			MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
			CYCLE=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;          \
			if test "X$$EXPECTED" != "X$$RESULT"; then                                \
				echo "==== FAILED: '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT'"; \
			fi;                                                                       \
			echo "==== ITERATION $$iter : '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLE [Cycles]";\
			if test "$$MIPS" != ""; then                                              \
				echo "$$MIPS" >> $$SIMLOGDIR/mips-list;                                 \
				echo "$$MIPS,$$TIME,$$CYCLE" >> $$SIMLOGDIR/assoc-list;                 \
			fi;                                                                       \
			rm -f $$SIMLOGDIR/$$TEST.log;                                             \
		done ;                                                                      \
		MIDDLE=`echo "(${ITERATIONS} / 2) + 1;"|bc`;                                \
		MEDIANMIPS=`sort -g $$SIMLOGDIR/mips-list | sed -n $$MIDDLE'p'`;            \
		MEDIANTIME=`grep $$MEDIANMIPS $$SIMLOGDIR/assoc-list | cut --delimiter="," -f 2`; \
		MEDIANCYCLE=`grep $$MEDIANMIPS $$SIMLOGDIR/assoc-list | cut --delimiter="," -f 3`; \
		echo "=== SPEED: '$$TEST' - $$MEDIANMIPS [MIPS] - $$MEDIANTIME [Seconds] - $$MEDIANCYCLE [Cycles]"; \
		echo "@$$TEST,$$MEDIANMIPS,$$MEDIANTIME,$$MEDIANCYCLE";                     \
		if test -d $$SIMLOGDIR ; then rm -rf $$SIMLOGDIR; fi                        \
	done

#--------------------------------------------------------------------------------
# @Target: test-performance-bioperf
# @Description: Performance test bioperf
#--------------------------------------------------------------------------------
_test-performance-bioperf:
	$(Verb) for t in ${_BIOPERF_TESTS} ; do                                        \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.XXXXXXXXXX`;                               \
		echo "=== RUNNING: 'bioperf/$$TEST'";                                       \
		pushd tests/bioperf/$$TEST >/dev/null;                                      \
		touch $$SIMLOGDIR/mips-list ; \
		touch $$SIMLOGDIR/assoc-list ; \
		for iter in {1..${ITERATIONS}} ; do                                         \
			${SIM}                                                                    \
					$(SIMEMTARG)                                                          \
					${SIMOPT}                                                             \
					--fast-tmp-dir=$$SIMLOGDIR/jit                                        \
					${SIMARGS}                                                            \
					${SIMRUNARG} `cat runcmd-small` &> $$SIMLOGDIR/$$TEST.log;            \
			RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;  \
			TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
			MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
			CYCLE=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;          \
			if test "X$$EXPECTED" != "X$$RESULT"; then                                \
				echo "==== FAILED: '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT'"; \
			fi;                                                                       \
			echo "==== ITERATION $$iter : '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLE [Cycles]";\
			if test "$$MIPS" != ""; then                                              \
				echo "$$MIPS" >> $$SIMLOGDIR/mips-list;                                 \
				echo "$$MIPS,$$TIME,$$CYCLE" >> $$SIMLOGDIR/assoc-list;                 \
			fi;                                                                       \
			rm -f $$SIMLOGDIR/$$TEST.log;                                             \
		done ;                                                                      \
		MIDDLE=`echo "(${ITERATIONS} / 2) + 1;"|bc`;                                \
		MEDIANMIPS=`sort -g $$SIMLOGDIR/mips-list | sed -n $$MIDDLE'p'`;            \
		MEDIANTIME=`grep $$MEDIANMIPS $$SIMLOGDIR/assoc-list | cut --delimiter="," -f 2`; \
		MEDIANCYCLE=`grep $$MEDIANMIPS $$SIMLOGDIR/assoc-list | cut --delimiter="," -f 3`; \
		echo "=== SPEED: '$$TEST' - $$MEDIANMIPS [MIPS] - $$MEDIANTIME [Seconds] - $$MEDIANCYCLE [Cycles]"; \
		echo "@$$TEST,$$MEDIANMIPS,$$MEDIANTIME,$$MEDIANCYCLE";                     \
		popd >/dev/null;                                                            \
		if test -d $$SIMLOGDIR ; then rm -rf $$SIMLOGDIR; fi                        \
	done


#--------------------------------------------------------------------------------
# @Target: test-performance-bioperf
# @Description: Performance test SPEC
#--------------------------------------------------------------------------------
_test-performance-spec:
	$(Verb) for t in $(_SPEC_TESTS) ; do                                        \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.XXXXXXXXXX`;                               \
		echo "=== RUNNING: spec/$$TEST in simulator";                            \
		pushd tests/spec/$$TEST >/dev/null;                                      \
		touch $$SIMLOGDIR/mips-list ; \
		touch $$SIMLOGDIR/assoc-list ; \
		for iter in {1..${ITERATIONS}} ; do                                         \
			${SIM}                                                                    \
					$(SIMEMTARG)                                                          \
					${SIMOPT}                                                             \
					--fast-tmp-dir=$$SIMLOGDIR/jit                                        \
					${SIMARGS}                                                            \
					${SIMRUNARG} `cat runcmd` &> $$SIMLOGDIR/$$TEST.log;            \
			RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;  \
			TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
			MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
			CYCLE=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;          \
			if test "X$$EXPECTED" != "X$$RESULT"; then                                \
				echo "==== FAILED: '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT'"; \
			fi;                                                                       \
			echo "==== ITERATION $$iter : '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLE [Cycles]";\
			if test "$$MIPS" != ""; then                                              \
				echo "$$MIPS" >> $$SIMLOGDIR/mips-list;                                 \
				echo "$$MIPS,$$TIME,$$CYCLE" >> $$SIMLOGDIR/assoc-list;                 \
			fi;                                                                       \
			rm -f $$SIMLOGDIR/$$TEST.log;                                             \
		done ;                                                                      \
		MIDDLE=`echo "(${ITERATIONS} / 2) + 1;"|bc`;                                \
		MEDIANMIPS=`sort -g $$SIMLOGDIR/mips-list | sed -n $$MIDDLE'p'`;            \
		MEDIANTIME=`grep $$MEDIANMIPS $$SIMLOGDIR/assoc-list | cut --delimiter="," -f 2`; \
		MEDIANCYCLE=`grep $$MEDIANMIPS $$SIMLOGDIR/assoc-list | cut --delimiter="," -f 3`; \
		echo "=== SPEED: '$$TEST' - $$MEDIANMIPS [MIPS] - $$MEDIANTIME [Seconds] - $$MEDIANCYCLE [Cycles]"; \
		echo "@$$TEST,$$MEDIANMIPS,$$MEDIANTIME,$$MEDIANCYCLE";                     \
		popd >/dev/null;                                                            \
		if test -d $$SIMLOGDIR ; then rm -rf $$SIMLOGDIR; fi                        \
	done                                                                              
