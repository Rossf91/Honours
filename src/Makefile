# ------------------------------------------------------------------------------
# Generic Definitions
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
Svnversion := @svnversion

# ------------------------------------------------------------------------------
# Variable definitions
# ------------------------------------------------------------------------------

TARGET_ARCH   = linux

MODULE        = arcsim

CC            = gcc
CXX           = g++

# Only in verbose mode do we show warnings
ifdef VERBOSE
  CXXWARNINGS   = -Wswitch -Wreturn-type -Wparentheses -Wconversion -Wsign-compare  -Wno-format -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wno-deprecated-declarations -Wno-invalid-offsetof
endif

OTHER         = -DCYCLE_ACC_SIM -DLLVM_VERSION=29  $(SVNDEV)

CXXFLAGS      = -O2 -m32 -mmmx -msse2 -fPIC -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe -g -O2 -flax-vector-conversions -DHAVE_MMX -DHAVE_SSE2 -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS $(OTHER) $(CXXWARNINGS)
CFLAGS        = -O2 -m32 -mmmx -msse2 -fPIC -pipe -g -O2 -flax-vector-conversions -DHAVE_MMX -DHAVE_SSE2 -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS   $(OTHER)
LDFLAGS       = -m32  -rdynamic 

EXTRA_SRCS    = 
EXTRA_INCDIRS = 
EXTRA_LIBS    =  -lc -ldl
EXTRA_LIBDIRS = 

ELFIO         = /group/project/arc_tools/arcsim/sys/i386/elfio/1.0.3
LLVM          = /group/project/arc_tools/arcsim/sys/i386/llvm/RELEASE_29
LLVM_VERSION  = 29
METAWARE      = 

# Local include path and ELFIO include path
##
INCDIR= -I../inc -I$(ELFIO)/include -I$(LLVM)/include $(EXTRA_INCDIRS)

# Append optional MetaWare MDB headers
##
ifneq ($(METAWARE),)
INCDIR:= $(INCDIR) -I$(METAWARE)/mdb/inc
endif


# The following library search paths are used when linking the standalone
# simulator executable.
##
LIBDIR= -L. -L.. -L$(ELFIO)/lib -L$(LLVM)/lib $(EXTRA_LIBDIRS)

## List of libraries for creating the standalone simulator executable.
###

# Clang libraries
##
CLANG_LIBS= \
  -lclangCodeGen      -lclangFrontend       -lclangDriver \
  -lclangIndex        -lclangParse          -lclangSema   \
  -lclangAnalysis     -lclangAST            -lclangLex    \
  -lclangBasic        -lclangRewrite        -lclangFrontendTool \
  -lclangSerialization \

# LLVM libraries
##
ifeq ($(LLVM_VERSION),30)
LLVM_LIBS= \
  -lLLVMX86Disassembler -lLLVMipo -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc         \
  -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMMCParser -lLLVMX86AsmPrinter -lLLVMX86Utils     \
  -lLLVMX86Info -lLLVMJIT -lLLVMExecutionEngine -lLLVMCodeGen -lLLVMScalarOpts              \
  -lLLVMInstCombine -lLLVMTransformUtils -lLLVMipa -lLLVMipo -lLLVMAnalysis -lLLVMTarget    \
  -lLLVMMC -lLLVMCore -lLLVMSupport
else
LLVM_LIBS= \
  -lLLVMX86Disassembler -lLLVMipo -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMSelectionDAG      \
  -lLLVMAsmPrinter -lLLVMMCParser -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMX86Info -lLLVMJIT  \
  -lLLVMExecutionEngine -lLLVMCodeGen -lLLVMScalarOpts -lLLVMInstCombine -lLLVMTransformUtils \
  -lLLVMipa -lLLVMAnalysis -lLLVMTarget -lLLVMMC -lLLVMCore -lLLVMSupport
endif

# All libraries we depend on
##
LIBS  = -lELFIO           \
        $(CLANG_LIBS)     \
        $(LLVM_LIBS)   \
        -lpthread         \
        -lm               \
        $(EXTRA_LIBS)

# ------------------------------------------------------------------------------
# Simulator sources
# ------------------------------------------------------------------------------
SIM_SRCS = \
	Assertion.cpp \
	util/Log.cpp \
	util/OutputStream.cpp \
	util/Counter.cpp \
	util/CounterTimer.cpp \
	util/Histogram.cpp \
	util/MultiHistogram.cpp \
	util/TraceStream.cpp \
	util/CodeBuffer.cpp \
	util/Allocate.cpp \
	util/SymbolTable.cpp \
	util/Os.cpp \
	util/Util.cpp \
	util/Zone.cpp \
	concurrent/Thread.cpp \
	concurrent/Mutex.cpp \
	concurrent/ConditionVariable.cpp \
	util/system/Timer.cpp \
	util/system/SharedLibrary.cpp \
	ioc/Context.cpp \
	ioc/ContextItemId.cpp \
	ioc/ContextItemInterface.cpp \
	ioc/ContextItemInterfaceFactory.cpp \
	ipt/IPTManager.cpp \
	arch/SimOptions.cpp \
	arch/IsaOptions.cpp \
	arch/PageArch.cpp \
	arch/CacheArch.cpp \
	arch/SpadArch.cpp \
	arch/MmuArch.cpp \
	arch/Configuration.cpp \
	sys/cpu/PageCache.cpp \
	sys/cpu/CounterManager.cpp \
	sys/cpu/EiaExtensionManager.cpp \
	sys/cpu/CcmManager.cpp \
	sys/cpu/processor.cpp \
	sys/cpu/processor-auxs.cpp \
	sys/cpu/processor-memory.cpp \
	sys/cpu/processor-interrupts.cpp \
	sys/cpu/processor-timers.cpp \
	sys/mmu/Mmu.cpp \
	sys/aps/Actionpoints.cpp \
	sys/smt/Smart.cpp \
	sys/mem/RoMemory.cpp \
	sys/mem/RaMemory.cpp \
	sys/mem/CcMemory.cpp \
	sys/mem/BlockData.cpp \
	sys/mem/Memory.cpp \
	isa/arc/Dcode.cpp \
	isa/arc/DcodeCache.cpp \
	isa/arc/Disasm.cpp \
	isa/arc/Opcode.cpp \
	system.cpp \
	trap.cpp \
	ise/eia/EiaExtension.cpp \
	ise/eia/EiaInstruction.cpp \
	ise/eia/EiaConditionCode.cpp \
	ise/eia/EiaCoreRegister.cpp \
	ise/eia/EiaAuxRegister.cpp \
	mem/MemoryDevice.cpp \
	mem/dma/DMACloselyCoupledMemoryDevice.cpp \
	mem/ccm/CCMemoryDevice.cpp \
	mem/mmap/IODevice.cpp \
	mem/mmap/IODeviceManager.cpp \
	mem/mmap/IODeviceUart.cpp \
	mem/mmap/IODeviceNull.cpp \
	uarch/bpu/BranchPredictorFactory.cpp \
	uarch/bpu/BranchPredictorOracle.cpp \
	uarch/bpu/BranchPredictorTwoLevel.cpp \
	uarch/memory/LatencyUtil.cpp \
	uarch/memory/LatencyCache.cpp \
	uarch/memory/CacheModel.cpp \
	uarch/memory/ScratchpadFactory.cpp \
	uarch/memory/CcmModel.cpp \
	uarch/memory/MainMemoryModel.cpp \
	uarch/memory/MemoryModel.cpp \
	uarch/memory/WayMemorisation.cpp \
	uarch/memory/WayMemorisationFactory.cpp \
	uarch/processor/ProcessorPipelineFactory.cpp \
	uarch/processor/ProcessorPipelineSkipjack.cpp \
	uarch/processor/ProcessorPipelineEncore5.cpp \
	uarch/processor/ProcessorPipelineEncore7.cpp \
	profile/BlockEntry.cpp \
	profile/PageProfile.cpp \
	profile/PhysicalProfile.cpp \
	translate/TranslationWorkUnit.cpp \
	translate/TranslationModule.cpp \
	translate/TranslationCache.cpp \
	translate/TranslationManager.cpp \
	translate/TranslationWorker.cpp \
	translate/TranslationOptManager.cpp \
	translate/TranslationEmit.cpp \
	translate/TranslateBlock.cpp \
	translate/TranslationRuntimeApi.cpp \
	api/api_funs.cpp \
	api/mem/api_mem.cpp \
	api/ioc/api_ioc.cpp \
	api/ipt/api_ipt.cpp \
	api/prof/api_prof.cpp \
	api/irq/api_irq.cpp \
	$(EXTRA_SRCS)

TMP_OBJS = $(SIM_SRCS:.cpp=.o)
SIM_OBJS = $(TMP_OBJS:.c=.o)

LSIMNAME    = sim
LIBSIM_OBJS = $(SIM_OBJS)

SRCS =  $(addprefix ../src/,$(SIM_SRCS) main.cpp)
OBJS = $(SIM_OBJS) main.o

# ------------------------------------------------------------------------------
# TARGET NAMES
# ------------------------------------------------------------------------------

BIN_TGT_DIR=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/bin
LIB_TGT_DIR=/afs/inf.ed.ac.uk/user/s09/s0903605/Downloads/trunk/lib

EXE    = ${BIN_TGT_DIR}/$(MODULE)

LSIMLIB= ${LIB_TGT_DIR}/lib$(LSIMNAME).so
ASIMLIB= ${LIB_TGT_DIR}/lib$(LSIMNAME).a

REVFILE= revision.txt

# ------------------------------------------------------------------------------
# REVISION NUMBERING 
#   Get the svn revision number for version tracking purposes.
#   If this instance of the source tree has been exported from svn, then the
#   svnversion command will return 'exported', and in this case we expect 
#   the revision.txt file to have been created at the time of exportation.
# ------------------------------------------------------------------------------
SVNVER := $(shell svnversion -n .)

ifeq ($(SVNVER),exported)
SVNDEV = -D'SVN_REV=$(shell cat $(REVFILE))'
else 
SVNDEV = -D'SVN_REV=$(SVNVER)'
endif

# ------------------------------------------------------------------------------
# TARGETS
# ------------------------------------------------------------------------------
.SUFFIXES: .c .cpp .o .x .lo

all: ${BIN_TGT_DIR} ${LIB_TGT_DIR} ${OBJS} ${LIBSIM_OBJS} ${EXE} ${LSIMLIB} ${ASIMLIB}

${BIN_TGT_DIR}:
	$(Verb) mkdir $@

${LIB_TGT_DIR}:
	$(Verb) mkdir $@

${LSIMLIB}: ${LIBSIM_OBJS}
	$(Echo) "arcsim: Creating Dynamic Library '${LSIMLIB}'"
	$(Verb) $(CXX) -m32 -shared -fPIC ${LDFLAGS} -o $@ -fPIC ${LIBSIM_OBJS} ${LIBDIR} ${LIBS}
	$(Echo) "arcsim: ===== Finished Dynamic Library '${LSIMLIB}'"

${ASIMLIB}: ${LIBSIM_OBJS} 
	$(Echo) "arcsim: Creating Archive '${ASIMLIB}'"
	$(Verb) ${AR} -cr ${ASIMLIB} ${LIBSIM_OBJS}

$(EXE): ${OBJS}
	$(Echo) "arcsim: Linking Executable '${EXE}'"
	$(Verb) $(CXX) ${LDFLAGS} -o $@ ${OBJS} ${LIBDIR} ${LIBS}
	$(Echo) "arcsim: ===== Finished Linking Executable '${EXE}'"

.cpp.lo:
	$(Verb) libtool --mode=compile $(CXX) $(CXXFLAGS) $(INCDIR) -c $<

.c.lo:
	$(Verb) libtool --mode=compile $(CC) $(CFLAGS) $(INCDIR) -c $<

.cpp.o: %.o: %.cpp
	$(Echo) "arcsim: Compiling $<"
	$(Verb) $(CXX) $(CXXFLAGS) $(INCDIR) -o $@ -c $<

.c.o: %.o: %.c
	$(Echo) "arcsim: Compiling $<"
	$(Verb) $(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

clean::
	$(Echo) "arcsim: Cleaning up"
	$(Verb) rm -f *~ $(EXE) core *.a *.so *.sl
	$(Verb) find . -name "*.o" |xargs rm -f 

Makefile.deps:
	$(Echo) "arcsim: Creating Dependencies $@"
	$(Verb) $(CXX) $(CFLAGS) $(INCDIR) -M $(SRCS) >> $@

distclean: clean
	$(Verb) rm -f Makefile.deps

include Makefile.deps
