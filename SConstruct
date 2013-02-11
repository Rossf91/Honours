# SConstruct - build script for building arcsim with scons

# Imports for filesystem operations, and argument checking.
import os, sys

# Get a handle for our build environment
env = Environment(ENV = os.environ)

# Without this we must build the objects for libsim with SharedObject() instead of Object()!
env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1

# Default variable values
EXECUTABLE_NAME = "bin/arcsim"
SHAREDLIB_NAME = "libsim" # scons can work out the extension

# Location where configuration settings will be stored
STORED_CONFIG_FILE = "config.py" 

# These will be changed automatically if you specify use64bit!
ARCH="i386"
MACHINE="-m32"

# Set the build root to the current working directory
BUILDROOT = os.getcwd()

# -----
# Function Definitions
# -----

# Function for handling message printing, so we can modify it later if need be.
def print_msg(msg):
	print "arcsim: " + msg

# Get the SVN revision number of these sources.
# TODO: Make this work on Windows as well?
def get_svn_rev():
	return os.popen('svn info | grep "Revision:" | awk \'{ print $2 }\'').read().strip()

# Check if path exists. If not, quit.
def ensure_path_exists(path, name):
	if are_we_building() and not os.path.isdir(path):
		print_msg("ERROR: Could not find " + name + " directory: " + path)
		Exit(1)
	
# Scan argv for certain flags, to work out if we're building.
not_building_flags = ['-h', '-c']
# If any of the above flags are in argv, return false, because we're not building...
def are_we_building():
	for flag in not_building_flags:
		if flag in sys.argv:
			return False
	return True

# When defines of the form -DDEFINE=value, if you want your string to be emitted into the C code as
# a proper string, then use this function to wrap it with proper quotes.
def conv_to_cstr(string):
	return ('\\"' + string + '\\"')

# LIST OF SOURCE FILES
#
sources = Split("""
util/Log.cpp
util/OutputStream.cpp
util/Counter.cpp
util/Histogram.cpp
util/MultiHistogram.cpp
util/TraceStream.cpp
util/CodeBuffer.cpp
util/Allocate.cpp
concurrent/Thread.cpp
concurrent/Mutex.cpp
concurrent/ConditionVariable.cpp
util/system/Timer.cpp
util/system/SharedLibrary.cpp
ioc/Context.cpp
ioc/ContextItemId.cpp
ioc/ContextItemInterface.cpp
ioc/ContextItemInterfaceFactory.cpp
arch/SimOptions.cpp
arch/IsaOptions.cpp
arch/PageArch.cpp
arch/CacheArch.cpp
arch/SpadArch.cpp
arch/Configuration.cpp
sys/cpu/ProcessorCounterContext.cpp
sys/cpu/PageCache.cpp
sys/cpu/EiaExtensionManager.cpp
sys/cpu/CcmManager.cpp
sys/cpu/processor.cpp
sys/cpu/processor-auxs.cpp
sys/cpu/processor-memory.cpp
sys/cpu/processor-interrupts.cpp
sys/cpu/processor-timers.cpp
sys/mmu/Mmu.cpp
sys/aps/Actionpoints.cpp
sys/smt/Smart.cpp
sys/mem/RoMemory.cpp
sys/mem/RaMemory.cpp
sys/mem/CcMemory.cpp
sys/mem/BlockData.cpp
sys/mem/Memory.cpp
isa/arc/Disasm.cpp
isa/arc/Dcode.cpp
isa/arc/DcodeCache.cpp
system.cpp
trap.cpp
ise/eia/EiaExtension.cpp
ise/eia/EiaInstruction.cpp
ise/eia/EiaConditionCode.cpp
ise/eia/EiaCoreRegister.cpp
ise/eia/EiaAuxRegister.cpp
mem/MemoryDevice.cpp
mem/dma/DMACloselyCoupledMemoryDevice.cpp
mem/ccm/CCMemoryDevice.cpp
mem/mmap/IODevice.cpp
mem/mmap/IODeviceManager.cpp
mem/mmap/IODeviceUart.cpp
mem/mmap/IODeviceNull.cpp
uarch/bpu/BranchPredictorFactory.cpp
uarch/bpu/BranchPredictorOracle.cpp
uarch/bpu/BranchPredictorTwoLevel.cpp
uarch/memory/LatencyUtil.cpp
uarch/memory/LatencyCache.cpp
uarch/memory/CacheModel.cpp
uarch/memory/ScratchpadFactory.cpp
uarch/memory/CcmModel.cpp
uarch/memory/MainMemoryModel.cpp
uarch/memory/MemoryModel.cpp
uarch/memory/WayMemorisation.cpp
uarch/memory/WayMemorisationFactory.cpp
uarch/processor/ProcessorPipelineFactory.cpp
uarch/processor/ProcessorPipelineSkipjack.cpp
uarch/processor/ProcessorPipelineEncore5.cpp
uarch/processor/ProcessorPipelineEncore7.cpp
profile/BlockProfile.cpp
profile/PageProfile.cpp
profile/PhysicalProfile.cpp
translate/TranslationWorkUnit.cpp
translate/TranslationModule.cpp
translate/TranslationCache.cpp
translate/TranslationManager.cpp
translate/TranslationWorker.cpp
translate/TranslationOptManager.cpp
translate/TranslationEmit.cpp
translate/TranslateBlock.cpp
translate/jit_funs.cpp
api/api_funs.cpp
api/mem/api_mem.cpp
""")

# This function selects the correct libraries to link against, depending on
# which LLVM version we are using.
def setup_libraries(using_llvm30):
	clang_libs = """
clangCodeGen clangFrontend clangDriver clangIndex clangParse clangSema
clangAnalysis clangAST clangLex clangBasic clangRewrite clangFrontendTool
clangSerialization
	"""
	
	llvm30_libs = """
LLVMX86Disassembler LLVMipo LLVMX86AsmParser LLVMX86CodeGen LLVMX86Desc      
LLVMSelectionDAG LLVMAsmPrinter LLVMMCParser LLVMX86AsmPrinter
LLVMX86Utils LLVMX86Info LLVMJIT LLVMExecutionEngine LLVMCodeGen
LLVMScalarOpts LLVMInstCombine LLVMTransformUtils LLVMipa LLVMipo 
LLVMAnalysis LLVMTarget LLVMMC LLVMCore LLVMSupport
	"""
	
	llvm29_libs = """
LLVMX86Disassembler LLVMipo LLVMX86AsmParser LLVMX86CodeGen LLVMSelectionDAG
LLVMAsmPrinter LLVMMCParser LLVMX86AsmPrinter LLVMX86Utils LLVMX86Info
LLVMJIT LLVMExecutionEngine LLVMCodeGen LLVMScalarOpts LLVMInstCombine
LLVMTransformUtils LLVMipa LLVMAnalysis LLVMTarget LLVMMC LLVMCore LLVMSupport
	"""

	llvm_libs = llvm29_libs
	if (using_llvm30):
		llvm_libs = llvm30_libs

	libs = "ELFIO " + clang_libs + llvm_libs + "pthread m"

  # Split() converts a string of whitespace-delimited entries to a list.
	return Split(libs)

# LIST OF COMPILE FLAGS
#
compile_flags = Split("""
-O2 -g -mmmx -msse2 -fPIC -fomit-frame-pointer -fno-exceptions -fno-rtti 
-pipe -Wswitch -Wreturn-type -Wparentheses -Wconversion -Wsign-compare
-Wno-format -Wunused-function -Wunused-label -Wunused-value -Wunused-variable 
-Wno-deprecated-declarations -Wno-invalid-offsetof
""")

# LIST OF DEFINES
#
defines = Split("""
HAVE_MMX HAVE_SSE2 _GNU_SOURCE __STDC_LIMIT_MACROS __STDC_CONSTANT_MACROS
""")

# LIST OF DEFINES WITH ASSOCIATED VALUES
#
value_defines = { \
"DEFAULT_ISA_FILE"      : conv_to_cstr(BUILDROOT + '/etc/encore.isa'), \
"DEFAULT_SYS_ARCH_FILE" : conv_to_cstr(BUILDROOT + '/etc/encore.arc'), \
"JIT_CC"                : conv_to_cstr('gcc'), \
"JIT_CCOPT"             : '', \
"loff_t"                : 'off_t', \
"SVN_REV"               : get_svn_rev() \
}

# LISTS TO BE SET UP AFTER PLATFORM SELECTION
# Will contain list of flags to use when linking.
link_flags = []
# Will contain list of flags to use when compiling shared objects - i.e. when using the external JIT.
shared_compile_flags = []
# Will contain list of flags to use when linking the shared library.
shared_link_flags = []

# ---
# CONFIGURATION OPTION CHECKING
#  - Modify the previously defined lists, depending on configuration options.
# ---

# Detect if --clear-config has been given as an argument.
# If so, delete the stored configuration file, and exit.
if "--clear-config" in sys.argv:
	if os.path.isfile(STORED_CONFIG_FILE):
		os.remove(STORED_CONFIG_FILE)
		print_msg("Removed configuration file. Quitting.")
		Exit(0)
	else:
		print_msg("ERROR: Cannot remove configuration file - doesn't exist!")
		Exit(1)
	
# Choose where to read our variables from:
# - if it exists, the stored configuration file
# - otherwise, the command line invokation
vars = 0
if os.path.isfile(STORED_CONFIG_FILE):
	print_msg("Reading configuration options from " + STORED_CONFIG_FILE + " ...")
	vars = Variables(STORED_CONFIG_FILE)
else:
	vars = Variables()

# Define all the configuration options.
vars.AddVariables(
    BoolVariable('verbose', 'Show invoked commands.', False),
		BoolVariable('use64bit', 'Build 64-bit simulator.', False),
		BoolVariable('llvm30', 'Enable use of LLVM 3.0. (Transitional)', False),
    BoolVariable('cycleacc', 'Build cycle-accurate simulator.', False),
		BoolVariable('regtrack', 'Build register-tracking enabled simulator.', False),
		BoolVariable('bpumodel', 'Build simulator supporting various BPU models.', False),
		BoolVariable('cosim', 'Build co-simulation enabled simulator.', False),
		BoolVariable('verify', 'Verification options enabled.', False),
		BoolVariable('iodevs', 'Enable memory mapped devices.', False),
		PathVariable('llvm', 'Path to LLVM installation.', "", PathVariable.PathAccept),
		PathVariable('elfio', 'Path to ELFIO installation.', "", PathVariable.PathAccept),
		PathVariable('metaware', 'Path to MetaWare installation.', "", PathVariable.PathAccept),
		)

# Check if unknown variables have been given.
unknown = vars.UnknownVariables()
if unknown:
	# We detected unknown variables - complain and exit.
	print "Unknown variables: ", unknown.keys()
	print "Use -h to see legal variables!"
	Exit(1)
else:
	# All clear - set up the env object with the provided arguments
	# - e.g. if use64bit=true was given, then env['use64bit'] will now be set to True
	vars.Update(env)

# VERBOSE: set the relevant command strings (*COMSTR) appropriately.
if not env['verbose']:
  env['ARCOMSTR'] = "arcsim: Archiving $TARGET"
  env['CCCOMSTR'] = "arcsim: Compiling $TARGET"
  env['CXXCOMSTR'] = "arcsim: Compiling $TARGET"
  env['SHLINKCOMSTR'] = "arcsim: Linking shared $TARGET"
  env['LINKCOMSTR'] = "arcsim: Linking $TARGET"
  env['RANLIBCOMSTR'] = "arcsim: Indexing $TARGET"
  env['TARCOMSTR'] = "arcsim: Creating $TARGET"

# USE64BIT: 
if env['use64bit']:
	MACHINE = "-m64"
	ARCH = "x86_64"

# Now that we know which architecture we're using, update the compile flags.
compile_flags.append(MACHINE)

# LLVM30:
value_defines["LLVM_VERSION"] = "29"
# when llvm30 = true, do this:
if env['llvm30']:
	value_defines["LLVM_VERSION"] = "30"

# Now that we know which LLVM version we're using, update the libraries.
libraries = setup_libraries(env['llvm30'])

# CYCLEACC:
if env['cycleacc']:
	defines.append("CYCLE_ACC_SIM")

# REGTRACK:
if env['regtrack']:
	defines.append("REGTRACK_SIM")

# BPUMODEL:
if env['bpumodel']:
	defines.append("ENABLE_BPRED")

# COSIM:
if env['cosim']:
	defines.append("COSIM_SIM")

# VERIFY:
if env['verify']:
	defines.append("VERIFICATION_OPTIONS")

# IODEVS:
if env['iodevs']:
	defines.append("ENABLE_IO_DEVICES")
	env.ParseConfig("pkg-config --cflags gdk-2.0 gtkglext-1.0")
	env.ParseConfig("pkg-config --libs-only-l gdk-2.0 gtkglext-1.0")
	env.ParseConfig("pkg-config --libs-only-L gdk-2.0 gtkglext-1.0")
	sources.extend(Split("""
	mem/mmap/IODeviceIrq.cpp
	mem/mmap/IODeviceKeyboard.cpp
	mem/mmap/IODeviceScreen.cpp
	mem/mmap/IODeviceSound.cpp
	"""))

# LLVM: Set LLVM_LOC appropriately, using a default if not set explicitly. 
LLVM_LOC  = "/group/project/arc_tools/arcsim/sys/" + ARCH + "/llvm/RELEASE_29"
if env['llvm'] != "":
	LLVM_LOC = env['llvm']
ensure_path_exists(LLVM_LOC, "LLVM")

# ELFIO: Set ELFIO_LOC appropriately, using a default if not set explicitly. 
ELFIO_LOC = "/group/project/arc_tools/arcsim/sys/" + ARCH + "/elfio/1.0.3" 
if env['elfio'] != "":
	ELFIO_LOC = env['elfio']
ensure_path_exists(ELFIO_LOC, "ELFIO")

# METAWARE: Add some sources, if metaware has been specified.
#  NB: this will be checked again later!
if env['metaware'] != "":
	sources.extend(Split("""
	api/arc/api_arcint.cpp
	api/arc/prof_count.cpp
	"""))

# ---
# PLATFORM CHECKING
#  - Modify the build options, depending on which platform we're using.
# ---

# Using the Linux platform
if sys.platform == "linux2": # TODO check if this is right
	link_flags.extend([MACHINE, "-rdynamic"])
	shared_compile_flags.extend([MACHINE, "-shared", "-fPIC"]) 
	shared_link_flags.extend([MACHINE, "-shared", "-fPIC", "--allow-shlib-undefined"]) 
	libraries.extend(["c", "dl"])
	if env['iodevs']:
		libraries.append("openal")

# Using the Mac OS X platform
elif sys.platform == "darwin":
	link_flags.extend(["-arch", ARCH, "-rdynamic"])
	shared_compile_flags.extend([MACHINE, "-dynamiclib", "-flat_namespace", "-undefined", "suppress"]) 
	shared_link_flags.extend(["-arch", ARCH, "-dylib", "-flat_namespace", "-undefined", "suppress"]) 
	libraries.extend(["c", "dl"])
	if env['iodevs']:
		link_flags.extend(["-framework", "OpenAL"]) 
		compile_flags.extend(["-FOpenAL"]) 

# Using the Windows platform
elif sys.platform == "win32": # TODO check if this is right
	link_flags.extend([MACHINE])

# config.h contains this define, which is set to the shared compile flags we set above
value_defines['JIT_CCOPT'] = conv_to_cstr('-pipe ' + ' '.join(shared_compile_flags))

# Finally, save the configuration file, if it doesn't exist!
if are_we_building() and not os.path.isfile(STORED_CONFIG_FILE):
	print_msg("Saving configuration options to " + STORED_CONFIG_FILE + " ...")
	vars.Save(STORED_CONFIG_FILE, env)

# -----
# SETUP BUILD ENVIRONMENT
# -----

# Make the fake config.h file. (Transitional, we will eventually get rid of the file.)
f = open("inc/config.h","w")
f.write("")
f.close()

# Add additional lib and include search paths
library_paths = [LLVM_LOC + "/lib",     ELFIO_LOC + "/lib"]
include_paths = [LLVM_LOC + "/include", ELFIO_LOC + "/include", "inc"]

# If we're building this for the metaware compiler, we add their headers to the include search path here.
if env['metaware'] != "":
	METAWARE_LOC = env['metaware'] + "/mdb/inc"
	include_paths.append(METAWARE_LOC)
	ensure_path_exists(METAWARE_LOC, "MetaWare")

# Prepend "src/" to the start of each source file name
sources = map(lambda s: "src/" + s, sources)

# Combine our "regular defines" and "defines with values" into one dictionary.
#  Regular defines must have the form { NAME: None } in the dictionary.
all_defines = dict(value_defines)
for define in defines:
	all_defines[define] = None
if "CPPDEFINES" in env:
	for define in env["CPPDEFINES"]:
		all_defines[define] = None

# Sets up the build environment according to our settings 
env.Append(CPPPATH = include_paths)
env.Append(LIBPATH = library_paths)
env.Append(CCFLAGS = compile_flags)
env.Append(LINKFLAGS = link_flags)
env.Append(LIBS = libraries)
env.Replace(CPPDEFINES = all_defines)

# -----
# PERFORM THE BUILD
# -----

# Create the object lists, from all the source files.
objects = env.Object(source = sources)
main_object = env.Object(source = ["src/main.cpp"])

# Specify that bin/arcsim is created from the objects in the object list, and the main_object
arcsim_target = env.Program(target = EXECUTABLE_NAME, source = objects + main_object)

# Specify that libsim is created from ONLY the objects in the objects list
#  (also, add the extra flags we should use when linking the shared lib)
libsim_target = env.SharedLibrary(target = SHAREDLIB_NAME, source = objects, LINKFLAGS = (link_flags + shared_link_flags))

# Set the default target, so we build bin/arcsim when we run scons without arguments
Default(arcsim_target)

# Define our other targets here
Alias('libsim', [libsim_target] )
Alias('all',    [arcsim_target, libsim_target] )

# Generates the help text, when you use -h
Help(vars.GenerateHelpText(env))
