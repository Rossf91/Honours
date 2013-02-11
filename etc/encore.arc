##
## encore.arc:  Default System Architecture File
##              This system architecture file has been calibrated for the 5-Stage
##              EnCore Castle32k chip that has been taped-out on 2009-10-16.
##
## chip:        /afs/inf.ed.ac.uk/group/project/pasta/encore/chip/tape-out/2009-10-16
## calibration: docs/2009-10-16-cycle-accurate-calibration.(xlsx|pdf)
##
## =============================================================================
##
## SYSTEM, MODULE and CORE sections:
## =================================
## - At least one SYSTEM, MODULE and CORE section must be defined.
## - All sections and definitions must be defined in the following order:
##   CACHE     <cache name>      <...> # Cache definition
##   CCM       <scratchpad name> <...> # CCM definition
##   MMU       <mmu name> <...>        # MMU definition
##   IFQ       <ifq name> <...>        # IFQ definition
##   BPU       <bpred name>      <...> # Branch predictor definition
##   CORE      <core name>       <...> # Start of a new Core section
##   MODULE    <module name>           # Start of a new Module section
##   SYSTEM    <system name>           # Start of the System section
##
##
## CACHE definition:
## =================
## - Replacement Policy: R = Random
## - Bus clock divisor is ignored if cache is used as L1 cpu cache
##   CACHE    <cache name> <size (Bytes)> <block offset (bits)> <associativity> <replacement policy> <higher bus width (bits)> <latency (cycles)> <higher bus clock divisor>
##
## CCM definition:
## ===============
##   CCM   <scratchpad name> <start address> <size (Bytes)> <data bus width (bits)> <latency (cycles)>
##
## MMU definition:
## ===============
## - Supported versions are (1|2|3)
## - Page size in bytes: default 8K, MMUv3 supports different sizes (1K,2K,4K,8K,16K)   - OPTIONAL PARAMETER
## - JTLB sets: default '128', MMU specification does not allow any other configuration - OPTIONAL PARAMETER
## - JTLB ways: default '2', MMUv3 supports '2' or '4' way configurations               - OPTIONAL PARAMETER
## 
##   MMU   <mmu name> <kind=0> <version> (<page size bytes> <# of JTLB sets> <# of JTLB ways>)

## MPU definition:
## ===============
## - Supported versions are (2)
##   MMU   <mmu name> <kind=1> <version> <number of regions, up to 16 in powers of 2>
##

## IFQ definition:
## ===============
## - Define an Instruction Fetch Queue
##   IFQ   <ifq name> <size>
##
## BPU definitions:
## ================
## - Define different types of branch predictor: A = GShare branch predictor, E = GSelect branch predictor, O = Oracle Predictor 
##   BPU   <bpu name> O <miss penalty (cycles)>  
##   BPU   <bpu name> A <BHT entries> <associativity> <sets> <RAS entries> <miss penalty (cycles)>  
##   BPU   <bpu name> E <BHT entries> <associativity> <sets> <RAS entries> <miss penalty (cycles)>  
##
## WPU definitions:
## ================
## - Cache way prediction (power)
##   WPU   <wpu name> <entries> <indices> <ways> <size> <block size> <phased (T/F)>
##
##
## CORE section:
## =============
##   CORE     <core name> <cpu clock ratio> <cpu data bus width (bits)> <pipeline stages> <warm-up period in cycles>
##
## MODULE section:
## ===============
##   MODULE      <module name>
##
## SYSTEM section:
## ===============
##   SYSTEM        <system name> <master clock freq (MHz)> <mem start address> <mem end address> <mem data bus width (bits)> <mem latency (cycles)> <mem bus clock divisor>
##
##
## ADD_CACHE statement:
## ====================
## - To add a private cache to a CORE or a shared cache to a MODULE or the SYSTEM
## - Cache types: I = instruction, D = data, U = unified
##   ADD_CACHE    <cache name> <type>
##
## ADD_CCM statement:
## ==================
## - To add Closely Coupled Memory to a CORE.
## - Memory type: I = iccm, D = dccm, U = uccm
##   ADD_CCM    <scratchpad name> <type>
##
## ADD_MMU statement:
## ==================
## - To add an MMU to a CORE.
##   ADD_MMU    <mmu name>
##
## ADD_IFQ statement:
## ==================
## - To add an IFQ to a CORE.
##   ADD_IFQ    <ifq name>
##
## ADD_BPU statement:
## ==================
## - To add a Branch Preditor Unit to a CORE.
## - If no branch prediction unit is specified perfect branch prediction is assumed.
##   ADD_BPU    <bpu name>
##
## ADD_WPU statement:
## ====================
## - To add Way Predictor Unit to a CORE.
## - WPU types: I = instruction, D = data
##   ADD_WPU    <wpu name> <type>
##
## ADD_CORE statement:
## ==================
## - To add cores to a MODULE
##   ADD_CORE    <core name> <number of cores>
##
## ADD_MODULE statement:
## ====================
## - To add modules to the SYSTEM
##   ADD_MODULE    <module name> <number of modules>
##
##  ============================================================================


## Cache Definition ------------------------------------------------------------
##  CACHE <cache name> <size (Bytes)> <block offset (bits)> <associativity>
##        <replacement policy> <higher bus width (bits)> <latency (cycles)>
##        <higher bus clock divisor>
#
#default cache sizes
#CACHE    32k_8w_l1_I  32768 5 8 R 32 1 1
#CACHE    32k_4w_l1_D  32768 5 4 R 32 1 1
#
#test cache sizes
CACHE    32k_8w_l1_I   256 5 1 R 32 1 1
CACHE    32k_4w_l1_D   256 5 1 R 32 1 1
CACHE    L2U           512 5 1 R 32 1 1

### Closely Coupled Memory Definition ------------------------------------------
## CCM  <CCM name> <start address> <size (Bytes)> <data bus width (bits)> <latency (cycles)>
##
## I-CCM sample configuration mapped to region 0
#
# CCM  32k_4w_l1_I_CCM 0          268435456 32 1
#
## D-CCM sample configuration mapped to region 8
#
# CCM  32k_4w_l1_D_CCM 2147483648 268435456 32 1

## Memory Management Unit Definition -------------------------------------------
##  MMU   <mmu name> <kind [mmu 0, mpu 1]> <version> (MMU:<page size bytes> <# of sets> <# of ways>) MPU:<num regions>
MMU mmu 0 1

## Instruction Fetch Queue Definition ------------------------------------------
##  IFQ   <ifq name> <size>
# IFQ ifq_8 8

## Core section (level 1) ------------------------------------------------------
#  CORE       <core name> <cpu clock ratio> <cpu data bus width (bits)>
#             <pipeline variant (EC5|EC7|SKIPJACK)> <warm-up period in cycles>
#
CORE          EC5_Castle32k_core 1 32 SKIPJACK 316
ADD_CACHE     32k_8w_l1_I I
ADD_CACHE     32k_4w_l1_D D
ADD_MMU       mmu

## ADD IFQ to core
# ADD_IFQ       ifq_8

## ADD CCMs to core
# ADD_CCM     32k_4w_l1_I_CCM I
# ADD_CCM     32k_4w_l1_D_CCM D

## Module section (level 2)
#  MODULE    <module name>
#
MODULE       EC5_Castle32k_module
ADD_CORE     EC5_Castle32k_core 1
ADD_CACHE    L2U U

## System section (level 3)
#  SYSTEM     <system name> <master clock freq (MHz)> <mem start address>
#             <mem end address> <mem data bus width (bits)>
#             <mem latency (cycles)> <mem bus clock divisor>
#
SYSTEM        EC5_Castle32k 250 0 4294967292 32 1 16
ADD_MODULE    EC5_Castle32k_module 1

##  ============================================================================
