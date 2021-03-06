#################################################################################
# mk/config.mk
#
# Definitions, variables, and configurations used by Makefiles
#################################################################################

#--------------------------------------------------------------------------------
# @Variables: $SIM, $SIMFLAGS
# @Description: Simulator and simulation flags
# @Example SIMOPTs for ArcSim '--trackregs --cycle --verbose --debug'
#--------------------------------------------------------------------------------

ifndef DRIVER_MDB

# Using ArcSim as a driver
##
SIM+=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/bin/arcsim
SIMOPT+=--verbose --fast
SIMARGS+=
SIMRUNARG=-e
SIMEMTARG=--emt

else

# Using mdb as a driver, utilising ArcSims library
##
SIM+=mdb
SIMOPT+=-cl \
	-dll=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib/libsim.so \
	-prop=nsim_fast=1 \
	-prop=nsim_isa_a6k=1 \
	-prop=nsim_isa_div_rem_option=1 \
	-prop=nsim_isa_code_density_option=1 \
	-prop=nsim_isa_bitscan_option=1 \
	-prop=nsim_isa_shift_option=3 \
	-prop=nsim_isa_atomic_option=1 \
	-prop=nsim_isa_fmt14=1 \
	-prop=nsim_isa_mpy_option=wlh1 \
	-mem=0xF00000 \
	-nohostlink_while_running -Xnoxcheck
SIMRUNARG=-run
SIMEMTARG=

endif

#--------------------------------------------------------------------------------
# @Variables: $LOG
# @Description: Directory in which log files should be stored.
#--------------------------------------------------------------------------------
LOG=log

#--------------------------------------------------------------------------------
# @Variables: miscellaneous stuff
#--------------------------------------------------------------------------------
RM:=rm -f
MAKE:=make

