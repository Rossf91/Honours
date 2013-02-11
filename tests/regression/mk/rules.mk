#################################################################################
# mk/rules.mk
#
# Test rules
#################################################################################


#--------------------------------------------------------------------------------
# @Target: setup
# @Description: Create log directories
#--------------------------------------------------------------------------------
setup:
	$(Verb) [ -d ${LOG} ] || mkdir ${LOG}

#--------------------------------------------------------------------------------
# @Target: test-regression
# @Description: Run regression test versions of benchmarks
#--------------------------------------------------------------------------------
test-default: setup
	$(Verb) _EEMBC_TESTS="$(EEMBC_DEFAULT_ITER)"     make _test-default-eembc
	$(Verb) _BIOPERF_TESTS="$(BIOPERF_DEFAULT_ITER)" make _test-default-bioperf
	$(Verb) _SPEC_TESTS="$(SPEC_DEFAULT_ITER)"       make _test-default-spec
	$(Verb)                                          make -C $(TOP)/../api clean test-default-api test-default-ipt-api
	$(Verb)                                          make _test-default-misc


#--------------------------------------------------------------------------------
# @Target: test-regression
# @Description: Run regression test versions of benchmarks
#--------------------------------------------------------------------------------
test-regression: setup
	$(Echo) "== Running Default Regression Test Suite"
	$(Verb) _EEMBC_TESTS="${EEMBC_REGRESSION}"     make _test-default-eembc    | tee    ${LOG}/$@.log
	$(Verb) _BIOPERF_TESTS="$(BIOPERF_REGRESSION)" make _test-default-bioperf  | tee -a ${LOG}/$@.log
	$(Verb) _SPEC_TESTS="$(SPEC_REGRESSION)"       make _test-default-spec     | tee -a ${LOG}/$@.log
	$(Verb)                                        make -C $(TOP)/../api clean test-default-api test-default-ipt-api | tee -a ${LOG}/$@.log
	$(Verb)                                        make _test-default-misc     | tee -a ${LOG}/$@.log
	$(Verb)                                        make _test-default-hex      | tee -a ${LOG}/$@.log
	$(Verb) awk 'BEGIN {F=0;} /FAILED/ {F++;}\
					END\
					{\
				    if(F==0) {\
				      printf("\n== ALL REGRESSION TESTS PASSED WITHOUT FAILURES\n");\
				    } else {\
				      printf("\n== FAILURE: %d REGRESSION TEST(S) FAILED\n", F);\
				    }\
					}' ${LOG}/$@.log

					

#--------------------------------------------------------------------------------
# @Target: test-regression-cycles
# @Description: Run regression test versions of benchmarks
#--------------------------------------------------------------------------------
test-regression-cycles-small: setup
	$(Echo) "== Running Cycle Accurate Default Regression Test Suite"
	$(Verb) _EEMBC_TESTS="${EEMBC_REGRESSION_CYCLES_SMALL}"     SIMOPT="--cycle" make _test-default-eembc-cycles    | tee    ${LOG}/$@.log
	$(Verb) _BIOPERF_TESTS="$(BIOPERF_REGRESSION_CYCLES_SMALL)" SIMOPT="--cycle" make _test-default-bioperf-cycles  | tee -a ${LOG}/$@.log
	$(Verb) awk 'BEGIN {F=0;} /FAILED/ {F++;}\
					END\
					{\
				    if(F==0) {\
				      printf("\n== ALL REGRESSION TESTS PASSED WITHOUT FAILURES\n");\
				    } else {\
				      printf("\n== FAILURE: %d REGRESSION TEST(S) FAILED\n", F);\
				    }\
					}' ${LOG}/$@.log



#--------------------------------------------------------------------------------
# @Target: test-regression-vendor-drop
# @Description: Run regression tests that must be passed BEFORE a vendor drop
#--------------------------------------------------------------------------------
test-regression-vendor-drop: setup
	$(Echo) "== Running Vendor Drop Regression Test Suite"
	$(Verb) _EEMBC_TESTS="${EEMBC_REGRESSION}"     SIMOPT="--fast-num-threads=3" make _test-default-eembc    | tee    ${LOG}/$@.log
	$(Verb) _BIOPERF_TESTS="$(BIOPERF_REGRESSION)" SIMOPT="--fast-num-threads=3" make _test-default-bioperf  | tee -a ${LOG}/$@.log
	$(Verb) _SPEC_TESTS="$(SPEC_REGRESSION)"       SIMOPT="--fast-num-threads=3" make _test-default-spec     | tee -a ${LOG}/$@.log
	$(Verb)                                                                      make _test-default-hex      | tee -a ${LOG}/$@.log
	$(Verb) DRIVER_MDB=1            SIMOPT="-prop=arcsim_fast-num-threads=3"     make _test-default-arcompactv2 | tee ${LOG}/$@.log
	$(Verb)                                                                      make -C $(TOP)/../api clean test-default-api test-default-ipt-api | tee -a ${LOG}/$@.log
	$(Verb)                                                                      make _test-default-misc     | tee -a ${LOG}/$@.log
	$(Verb) awk 'BEGIN {F=0;} /FAILED/ {F++;}\
					END\
					{\
				    if(F==0) {\
				      printf("\n== ALL VENDOR DROP REGRESSION TESTS PASSED WITHOUT FAILURES\n");\
				    } else {\
				      printf("\n== FAILURE: %d REGRESSION TEST(S) FAILED\n", F);\
				    }\
					}' ${LOG}/$@.log                                         

#--------------------------------------------------------------------------------
# @Target: test-a6k-regression
# @Description: Run ARCompact V2 test versions of benchmarks using MDB
#--------------------------------------------------------------------------------
test-a6k-regression: setup
	$(Echo) "== Running ARCompact V2 Regression Test Suite using MDB"
	$(Verb) _EEMBC_TESTS="$(EEMBC_A6K_REGRESSION)" make _test-default-arcompactv2 | tee ${LOG}/$@.log
	$(Verb) awk 'BEGIN {F=0;} /FAILED/ {F++;}\
					END\
					{\
				    if(F==0) {\
				      printf("\n== ALL REGRESSION TESTS PASSED WITHOUT FAILURES\n");\
				    } else {\
				      printf("\n== FAILURE: %d REGRESSION TEST(S) FAILED\n", F);\
				    }\
					}' ${LOG}/$@.log


#--------------------------------------------------------------------------------
# @Target: test-default-eembc
# @Description: Run eembc regression test binaries.
#--------------------------------------------------------------------------------
_test-default-eembc:
	$(Verb) for t in ${_EEMBC_TESTS} ; do                                            \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		${SIM} ${SIMOPT} ${SIMARGS}                                       \
			${SIMRUNARG} $(TOP)/tests/eembc/default-iterations/$$TEST &> $$SIMLOGDIR/$$TEST.log;\
		RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		CYCLES=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;           \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [EEMBC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLES [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [EEMBC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done


#--------------------------------------------------------------------------------
# @Target: test-default-eembc-cycles
# @Description: Run eembc regression test binaries.
#--------------------------------------------------------------------------------
_test-default-eembc-cycles:
	$(Verb) for t in ${_EEMBC_TESTS} ; do                                            \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		${SIM} ${SIMOPT} ${SIMARGS}                                       \
			${SIMRUNARG} $(TOP)/tests/eembc/default-iterations/$$TEST &> $$SIMLOGDIR/$$TEST.log;\
		RESULT=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [EEMBC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$RESULT [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [EEMBC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done


#--------------------------------------------------------------------------------
# @Target: test-default-bioperf
# @Description: Run bioperf regression test binaries.
#--------------------------------------------------------------------------------
_test-default-bioperf:
	$(Verb) for t in ${_BIOPERF_TESTS} ; do                                       \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd tests/bioperf/$$TEST >/dev/null;                                      \
		${SIM} ${SIMEMTARG} ${SIMOPT} ${SIMARGS} ${SIMRUNARG} `cat runcmd-small` &> $$SIMLOGDIR/$$TEST.log; \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		CYCLES=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;           \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [BIOPERF] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLES [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [BIOPERF] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done


#--------------------------------------------------------------------------------
# @Target: test-default-bioperf-cycles
# @Description: Run bioperf regression test binaries.
#--------------------------------------------------------------------------------
_test-default-bioperf-cycles:
	$(Verb) for t in ${_BIOPERF_TESTS} ; do                                       \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd tests/bioperf/$$TEST >/dev/null;                                      \
		${SIM} ${SIMEMTARG} ${SIMOPT} ${SIMARGS} ${SIMRUNARG} `cat runcmd-small` &> $$SIMLOGDIR/$$TEST.log; \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [BIOPERF] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$RESULT [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [BIOPERF] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done


#--------------------------------------------------------------------------------
# @Target: test-default-spec
# @Description: Run spec regression test binaries.
#--------------------------------------------------------------------------------
_test-default-spec:
	$(Verb) for t in ${_SPEC_TESTS} ; do                                             \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd tests/spec/$$TEST >/dev/null;                                         \
		${SIM} ${SIMEMTARG} ${SIMOPT} ${SIMARGS} ${SIMRUNARG} `cat runcmd` &> $$SIMLOGDIR/$$TEST.log; \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;            \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;            \
		CYCLES=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;           \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [SPEC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLES [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [SPEC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done


#--------------------------------------------------------------------------------
# @Target: test-default-spec-cycles
# @Description: Run spec regression test binaries.
#--------------------------------------------------------------------------------
_test-default-spec-cycles:
	$(Verb) for t in ${_SPEC_TESTS} ; do                                             \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd tests/spec/$$TEST >/dev/null;                                         \
		${SIM} ${SIMEMTARG} ${SIMOPT} ${SIMARGS} ${SIMRUNARG} `cat runcmd` &> $$SIMLOGDIR/$$TEST.log; \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;       \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;            \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;            \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [SPEC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$RESULT [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [SPEC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done
	

#--------------------------------------------------------------------------------
# @Target: test-default-arcompactv2
# @Description: Run eembc binaries with new SkipJack a.k.a. a6k ISA options
#--------------------------------------------------------------------------------
_test-default-arcompactv2:
	$(Verb) for V in small medium large ; do                                               \
		for T in ${EEMBC_A6K_REGRESSION} ; do                                         \
			TEST=`echo "$$T" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
			PARAMS=`echo "$$T" | awk 'BEGIN {FS=":"} {print $$2}'`;                      \
			SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
			pushd "$(TOP)/tests/eembc/a6k-variant/$${V}-iterations/$${TEST}" >/dev/null;\
			${SIM} ${SIMOPT} ${SIMARGS}                                                 \
				${SIMRUNARG} bmark.out $$PARAMS &> $$SIMLOGDIR/$$TEST.log;                \
			cat bmark.bas | sed -e "/Iteration/d" -e "/Run Time/d" -e "/Duration/d" -e "/Time.*Iter/d" \
										> $$SIMLOGDIR/$$TEST.check; \
			cat $$SIMLOGDIR/$$TEST.log |  sed -e "/Debugger license/d" -e "/\*\*\*\ Ashling/d" \
										-e "/\*\*\*\ Say/d" -e "/Iteration/d" -e "/Run Time/d" \
										-e "/Duration/d" -e "/Time.*Iter/d" -e "/Elapsed Cycles/d" \
										-e "/Dwarf Translator/d" -e "/Unusual: Debug section/d" \
										-e "/Page\ limit\ /d" -e "/Port address changed/d" \
										-e "/Component error:/d" -e "/Failed to read Dwarf/d" -e "/GCLK/d" \
										-e "/ARC_DLL*/d" \
										> $$SIMLOGDIR/$$TEST.out;                       \
			popd >/dev/null;                                                            \
			diff -b $$SIMLOGDIR/$$TEST.check $$SIMLOGDIR/$$TEST.out > $$SIMLOGDIR/$$TEST.diff.out; \
			if test $$? = 0;                                                             \
				then                                                                       \
					echo "=== PASSED: [EEMBC-$${V}] '$$TEST'";                               \
					if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;                \
				else echo "=== FAILED: [EEMBC-$${V}] '$$TEST'";                            \
			fi;                                                                          \
		done \
	done


#--------------------------------------------------------------------------------
# @Target: test-default-misc
# @Description: Run misc. test cases exercising various parts of the code. 
#  AWK parameters: "binary:benchmark description:dir:arguments:instruction count:(F|W)"
#--------------------------------------------------------------------------------
_test-default-misc:
	$(Echo) "== Building EIA extensions for testing"
	$(Verb) make -C ../../$(TOP)/examples/eia build-all
	$(Verb) for t in ${MISC_TEST_CASES} ; do                                      \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		DESC=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                       \
		XPATH=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                      \
		XARGS=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$4}'`;                      \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$5}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$6}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd $(TOP)/$${XPATH}  >/dev/null;                                         \
		${SIM} --verbose $${XARGS} -e $${TEST} &> $$SIMLOGDIR/$$TEST.log;           \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		CYCLES=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;           \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [MISC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLES [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [MISC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done \


#--------------------------------------------------------------------------------
# @Target: test-default-misc-cycles
# @Description: Run misc. test cases exercising various parts of the code. 
#  AWK parameters: "binary:benchmark description:dir:arguments:instruction count:(F|W)"
#--------------------------------------------------------------------------------
_test-default-misc-cycles:
	$(Echo) "== Building EIA extensions for testing"
	$(Verb) make -C ../../$(TOP)/examples/eia build-all
	$(Verb) for t in ${MISC_TEST_CASES} ; do                                      \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		DESC=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                       \
		XPATH=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                      \
		XARGS=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$4}'`;                      \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$5}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$6}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd $(TOP)/$${XPATH}  >/dev/null;                                         \
		${SIM} --verbose $${XARGS} -e $${TEST} &> $$SIMLOGDIR/$$TEST.log;           \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [MISC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$RESULT [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [MISC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done \


#--------------------------------------------------------------------------------
# @Target: test-default-misc
# @Description: Run misc. test cases exercising various parts of the code. 
#  AWK parameters: "binary:benchmark description:dir:arguments:instruction count:(F|W)"
#--------------------------------------------------------------------------------
_test-default-hex:
	$(Verb) for t in ${HEX_TEST_CASES} ; do                                      \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		DESC=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                       \
		XPATH=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                      \
		XARGS=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$4}'`;                      \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$5}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$6}'`;                 \
		SIMLOGDIR=`mktemp -d /tmp/arcsim.regression.$$TEST.XXXXXXXXXX`;             \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		pushd $(TOP)/$${XPATH}  >/dev/null;                                         \
		${SIM} --verbose $${XARGS} -H $${TEST} &> $$SIMLOGDIR/$$TEST.log;           \
		popd >/dev/null;                                                            \
		RESULT=`awk '/^Total instructions/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;    \
		TIME=`awk '/^Simulation time/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		MIPS=`awk '/^Simulation rate/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;         \
		CYCLES=`awk '/^Cycle count/ {print $$4}' $$SIMLOGDIR/$$TEST.log`;           \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [MISC] '$$TEST' @ $$MIPS [MIPS] - $$TIME [Seconds] - $$CYCLES [Cycles]"; \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [MISC] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT' - @ $$MIPS [MIPS] - $$TIME [Seconds]";\
		fi;\
	done \


