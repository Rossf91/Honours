#                          Copyright Notice
#
#    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
#  The University Court of the University of Edinburgh. All Rights Reserved.
#
# =============================================================================

# ------------------------------------------------------------------------------
# Makefile for building MemoryDevice implementation extensions.
# ------------------------------------------------------------------------------

# Add additional MemoryDevice implementations to the following variable to build them
# automatically as shared libraries.
####
SRCS = \
	ExternalMemory/ExternalMemory.cpp


# ------------------------------------------------------------------------------

# This option tells the Makefiles to produce verbose output.
# It essentially prints the commands that make is executing
# VERBOSE = 1

# Adjust settings for verbose mode
ifndef VERBOSE
	Verb := @
endif

# Things we just assume are "there"
Echo := @echo

# ------------------------------------------------------------------------------

CXX           = g++

CXXFLAGS      = -O2 -m32 -mmmx -msse2 -fPIC -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe -g -O2 -flax-vector-conversions -DHAVE_MMX -DHAVE_SSE2 -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS $(CXXWARNINGS)
LDFLAGS       = -m32  -rdynamic 

INCDIR        = -I../../inc
LIBDIR        = -L../../lib

LIBS          = $(SRCS:.cpp=.so)

MEM_DEV       = ExternalMemory/ExternalMemory.so

# ------------------------------------------------------------------------------
# TARGETS
# ------------------------------------------------------------------------------

.SUFFIXES: .cpp .o .so

all: arcsim ${LIBS}

# Make sure top-level ArcSim is built
##
arcsim:
	$(Verb) make -s -C ../..

build-all:
	$(Verb) make clean
	$(Verb) make ${MEM_DEV}

.o.so: %.so: %.o
	$(Echo) "MEMDEV: Creating Dynamic MemoryDevice Library '$@'"
	$(Verb) $(CXX) -m32 -shared -fPIC ${LDFLAGS} -fPIC -o $@ $<

.cpp.o: %.o: %.cpp
	$(Echo) "MEMDEV: Compiling MemoryDevice '$<'"
	$(Verb) $(CXX) $(CXXFLAGS) $(INCDIR) -o $@ -c $<

clean::
	$(Echo) "MEMDEV: Cleaning up MemoryDevices"
	$(Verb) rm -rf *~ TestEia.x *.dSYM
	$(Verb) find . -name "*.o"  |xargs rm -f
	$(Verb) find . -name "*.so" |xargs rm -f

