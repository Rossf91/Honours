###############################################################################
#
# Simulation API Test Harness
#
###############################################################################

default=build-api

#--------------------------------------------------------------------------------
# @Variable: API_TEST_BINARIES
# @Description: API regression test binaries
#--------------------------------------------------------------------------------
API_TEST_BINARIES=\
	eembc-empty.x:392:F

#--------------------------------------------------------------------------------
# @Variable: API_IPT_TEST_BINARIES
# @Description: IPT API regression test binaries.
# @Format: name:iptcount:(F|W)
#--------------------------------------------------------------------------------
API_IPT_TEST_BINARY=ipt-api-test-bin.x:423962:F
API_IPT_API_TEST_ABOUT_TO_EXECUTE_INSTR=ipt-api-test-about-to-execute-bin.x:10:F
API_IPT_API_TEST_BEGIN_INSTR_EXEC=ipt-api-test-begin-instr-exec-bin.x
API_IPT_API_TEST_BEGIN_BASIC_BLOCK_EXEC=ipt-api-test-begin-basic-block-exec-bin.x

#--------------------------------------------------------------------------------
# @Target: api-test
# @Description: API regression test harness build rules
#--------------------------------------------------------------------------------

build-api: api-test ipt-api-test ipt-api-test-about-to-execute ipt-api-test-begin-instr-exec ipt-api-test-begin-basic-block-execute

api-test: api-test.c
	@echo "== Building Test Harness '$@' for simulation API - $^ "
	gcc -I/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/inc -L/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib -lsim $^ -o $@

ipt-api-test: ipt-api-test.c
	@echo "== Building Test Harness '$@' for IPT simulation API"
	gcc -I/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/inc -L/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib -lsim $^ -o $@

ipt-api-test-begin-instr-exec: ipt-api-test-begin-instr-exec.c
	@echo "== Building Test Harness '$@' for IPT simulation API"
	gcc -I/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/inc -L/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib -lsim $^ -o $@

ipt-api-test-about-to-execute: ipt-api-test-about-to-execute.c
	@echo "== Building Test Harness '$@' for IPT simulation API"
	gcc -I/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/inc -L/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib -lsim $^ -o $@

ipt-api-test-begin-basic-block-execute: ipt-api-test-begin-basic-block-exec.c
	@echo "== Building Test Harness '$@' for IPT simulation API"
	gcc -I/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/inc -L/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib -lsim $^ -o $@


#--------------------------------------------------------------------------------
# @Target: test-default-api
# @Description: Run API regression test binaries.
#--------------------------------------------------------------------------------
test-default-api: api-test
	@echo "== Running Test Harness using simulation API"
	@for t in ${API_TEST_BINARIES}; do                                            \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		EXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                   \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		LD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
		DYLD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
			./$< -e $$TEST &> $$TEST.log;                                             \
		RESULT=`awk '/^Total instructions/ {print $$4}' $$TEST.log`;                \
		if test "X$$EXPECTED" = "X$$RESULT";                                        \
			then                                                                      \
				echo "=== PASSED: [API-TEST] '$$TEST'";                                 \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [API-TEST] '$$TEST' - expected:'$$EXPECTED' actual:'$$RESULT'";\
		fi;\
	done

#--------------------------------------------------------------------------------
# @Target: test-default-ipt-api
# @Description: Run API regression test binaries for IPT API.
#--------------------------------------------------------------------------------
test-default-ipt-api: ipt-api-test ipt-api-test-about-to-execute ipt-api-test-begin-instr-exec ipt-api-test-begin-basic-block-execute
	@echo "== Running Test Harness using IPT simulation API"
	@for t in ${API_IPT_TEST_BINARY}; do                                        \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		IPTEXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		LD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
		DYLD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
			./ipt-api-test -e $$TEST &> $$TEST.log;                                   \
		IPTRESULT=`awk '/^Executed Instrumentation PoinTs:/ {print $$4}' $$TEST.log`;\
		if test "X$$IPTEXPECTED" = "X$$IPTRESULT";                                 \
			then                                                                      \
				echo "=== PASSED: [API-IPT-TEST] '$$TEST'";                           \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [API-IPT-TEST] '$$TEST' - expected:'$$IPTEXPECTED' actual:'$$IPTRESULT'";\
		fi;\
	done
	@for t in ${API_IPT_API_TEST_ABOUT_TO_EXECUTE_INSTR}; do                      \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		IPTEXPECTED=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$2}'`;                \
		FAILORWARN=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$3}'`;                 \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		LD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
		DYLD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
			./ipt-api-test-about-to-execute -e $$TEST &> $$TEST.log;                  \
		IPTRESULT=`awk '/^Executed Instrumentation PoinTs:/ {print $$4}' $$TEST.log`;\
		if test "X$$IPTEXPECTED" = "X$$IPTRESULT";                                 \
			then                                                                      \
				echo "=== PASSED: [API-IPT-TEST] '$$TEST'";                           \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== $$MESSAGE: [API-IPT-TEST] '$$TEST' - expected:'$$IPTEXPECTED' actual:'$$IPTRESULT'";\
		fi;\
	done
	@for t in ${API_IPT_API_TEST_BEGIN_INSTR_EXEC}; do                            \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		LD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
		DYLD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
			./ipt-api-test-begin-instr-exec -e $$TEST &> $$TEST.log;                  \
		OUTCOME=`echo $$?`; \
		if test "X$$OUTCOME" = "X0";                                 \
			then                                                                      \
				echo "=== PASSED: [API-IPT-TEST] '$$TEST'";                           \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== FAILED: [API-IPT-TEST] '$$TEST'";\
		fi;\
	done
	@for t in ${API_IPT_API_TEST_BEGIN_BASIC_BLOCK_EXEC}; do                            \
		TEST=`echo "$$t" | awk 'BEGIN {FS=":"} {print $$1}'`;                       \
		if test "X$$FAILORWARN" = "XW"; then MESSAGE="WARNING"; else MESSAGE="FAILED"; fi;\
		LD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
		DYLD_LIBRARY_PATH=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib  \
			./ipt-api-test-begin-basic-block-execute -e $$TEST &> $$TEST.log;                  \
		OUTCOME=`echo $$?`; \
		if test "X$$OUTCOME" = "X0";                                 \
			then                                                                      \
				echo "=== PASSED: [API-IPT-TEST] '$$TEST'";                           \
				if test -d "$$SIMLOGDIR" ; then rm -rf "$$SIMLOGDIR"; fi;               \
			else echo "=== FAILED: [API-IPT-TEST] '$$TEST'";\
		fi;\
	done


clean:
	@rm -rf api-test ipt-api-test ipt-api-test-about-to-execute ipt-api-test-begin-instr-exec ipt-api-test-begin-basic-block-execute *.dSYM

