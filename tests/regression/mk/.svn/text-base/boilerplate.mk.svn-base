#################################################################################
# mk/boilerplate.mk
#
# The ArcSim Trainer Boilerplate Makefile (borrowed from the GHC project)
#
# This one file should be included (directly or indirectly) by all Makefiles
# as follows:
#
# 	TOP=.
# 	include $(TOP)/mk/boilerplate.mk
#
# where the variable TOP should point to the top folder relative
# from where the Makefile lives.
#################################################################################

# This option tells the Makefiles to produce verbose output.
# It essentially prints the commands that make is executing
# VERBOSE = 1

# Adjust settings for verbose mode
ifndef VERBOSE
  Verb := @
endif

# Things we just assume are "there"
Echo := @echo

# -----------------------------------------------------------------------------
# @Variable: 'MAKEFLAGS'
# @Description: We want to disable all the built-in rules that make uses; having
# them just slows things down, and we write all the rules ourselves. Setting 
# .SUFFIXES to empty disables them all.
# -----------------------------------------------------------------------------
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# -----------------------------------------------------------------------------
# @Target: 'make all'
# @Description: This rule makes sure that "all" is the default target, regardless
# of where it appears. THIS RULE MUST REMAIN FIRST!!!!
# -----------------------------------------------------------------------------
default: all

# -----------------------------------------------------------------------------
# @Target: 'make show'
# @Description: Makefile debugging - to see the effective value used for a Makefile
# variable, do 'make show VALUE=MY_VALUE'
# -----------------------------------------------------------------------------
show:
	@echo '$(VALUE)="$($(VALUE))"'

# -----------------------------------------------------------------------------
# Misc bits

# -----------------------------------------------------------------------------
# @Variable: 'MAKEFLAGS'
# When using $(patsubst ...) and friends, you can't use a literal comma freely - 
# so we use ${comma} instead.
# -----------------------------------------------------------------------------
comma=,

# -----------------------------------------------------------------------------
# Now follow the pieces of boilerplate
#	The "-" signs tell make not to complain if they don't exist

include $(TOP)/mk/config.mk
include $(TOP)/mk/tests.mk
# rules*.mk should go last as they rely on variables defined in previous includes
include $(TOP)/mk/rules.mk
include $(TOP)/mk/rules-performance.mk
include $(TOP)/mk/rules-multicore.mk


