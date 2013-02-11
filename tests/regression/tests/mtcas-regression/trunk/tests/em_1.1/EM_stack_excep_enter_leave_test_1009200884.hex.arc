## syntax.specification.start===================================================
##
##
## SYSTEM, MODULE and CORE sections:
## =================================
## - At least one SYSTEM, MODULE and CORE section must be defined.
## - All sections and definitions must be defined in the following order:
##   CACHE     <cache name>      <...> # Cache definition
##   CCM       <scratchpad name> <...> # CCM definition
##   MMU       <mmu name> <...>        # MMU definition
##   BPU       <bpred name>      <...> # Branch predictor definition
##   CORE      <core name>       <...> # Start of a new Core section
##   MODULE    <module name>           # Start of a new Module section
##   SYSTEM    <system name>           # Start of the System section
##
## BPU definitions:
## ================
## - Define different types of branch predictor: N = always Not Taken, T = always Taken, A = uARC branch predictor
##   BPU  <bpu name> N <miss penalty (cycles)>
##   BPU  <bpu name> T <miss penalty (cycles)>
##   BPU  <bpu name> A <BHT entries> <associativity> <sets> <RAS entries> <miss penalty (cycles)>
##
## CACHE definition:
## =================
## - Replacement Policy: R = Random
## - Bus clock divisor is ignored if cache is used as L1 cpu cache
##   CACHE  <cache name> <size (Bytes)> <block offset (bits)> <associativity> <replacement policy> <higher bus width (bits)> <latency (cycles)> <higher bus clock divisor>
##
## CCM definition:
## ===============
##   CCM  <scratchpad name> <start address> <size (Bytes)> <data bus width (bits)> <latency (cycles)>
##
## MMU definition:
## ===============
## - Supported versions are (1|2|3)
## - Page size in bytes: default 8K, MMUv3 supports different sizes (1K,2K,4K,8K,16K)   - OPTIONAL PARAMETER
## - JTLB sets: default '128', MMU specification does not allow any other configuration - OPTIONAL PARAMETER
## - JTLB ways: default '2', MMUv3 supports '2' or '4' way configurations               - OPTIONAL PARAMETER
## 
##   MMU   <mmu name> <version> (<page size bytes> <# of JTLB sets> <# of JTLB ways>)
##
## IFQ definition:
## ===============
##   IFQ  <ifq name> <size>
##
## WPU definitions:
## ================
## - Cache way prediction (power)
##   WPU  <wpu name> <entries> <indices> <ways> <size> <block size> <phased (T/F)>
##
## CORE section:
## =============
##   CORE  <core name> <cpu clock ratio> <cpu data bus width (bits)> <pipeline stages> <warm-up period in cycles>
##
## MODULE section:
## ===============
##   MODULE  <module name>
##
## SYSTEM section:
## ===============
##   SYSTEM  <system name> <master clock freq (MHz)> <mem start address> <mem end address> <mem data bus width (bits)> <mem latency (cycles)> <mem bus clock divisor>
##
##
## ADD_BPU statement:
## ==================
## - To add a Branch Preditor Unit to a CORE.
## - If no branch prediction unit is specified perfect branch prediction is assumed.
##   ADD_BPU  <bpu name>
##
## ADD_CACHE statement:
## ====================
## - To add a private cache to a CORE or a shared cache to a MODULE or the SYSTEM
## - Cache types: I = instruction, D = data, U = unified
##   ADD_CACHE  <cache name> <type>
##
## ADD_CCM statement:
## ==================
## - To add Closely Coupled Memory to a CORE.
## - Memory type: I = iccm, D = dccm, U = uccm
##   ADD_CCM  <scratchpad name> <type>
##
## ADD_MMU statement:
## ==================
## - To add an MMU to a CORE.
##   ADD_MMU    <mmu name>
##
## ADD_IFQ statement:
## ==================
## - To add Instruction Fetch Queue to a CORE
##   ADD_IFQ  <ifq name>
##
## ADD_WPU statement:
## ====================
## - To add Way Predictor Unit to a CORE.
## - WPU types: I = instruction, D = data
##   ADD_WPU  <wpu name> <type>
##
## ADD_CORE statement:
## ==================
## - To add cores to a MODULE
##   ADD_CORE  <core name> <number of cores>
##
## ADD_MODULE statement:
## ====================
## - To add modules to the SYSTEM
##   ADD_MODULE  <module name> <number of modules>
##
##
## syntax.specification.end=====================================================


## architecture.start===========================================================

# Cache definition:
CACHE ICACHE 32768 6 4 R 32 1 1
CACHE DCACHE 32768 6 4 R 32 1 1

# Closely Coupled Memory definition:


# Instruction Fetch Queue definition:


# MPU definition:


# Core section (level 1):
CORE arcv2em_core 1 32 SKIPJACK 316

# Add Caches to core:
ADD_CACHE ICACHE I
ADD_CACHE DCACHE D

# ADD CCMs to core:


# Add IFQ to core:


# Add MPU to core:


# Module section (level 2):
MODULE arcv2em_module
ADD_CORE arcv2em_core 1

# System section (level 3):
SYSTEM arcv2em_system 250 0 4294967292 32 1 16
ADD_MODULE arcv2em_module 1

## architecture.end=============================================================
