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
##   MMU   <mmu name> <version>
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


## Cache definition
#  CACHE    <cache name> <size (Bytes)> <block offset (bits)> <associativity>
#           <replacement policy> <higher bus width (bits)> <latency (cycles)>
#           <higher bus clock divisor>
#

# Cache configuration
#
CACHE    32k_8w_l1_I  32768 5 8 R 32 1 1
CACHE    32k_4w_l1_D  32768 5 4 R 32 1 1

## Closely Coupled Memory definition
# CCM   <CCM name> <start address> <size (Bytes)> <data bus width (bits)>
#       <latency (cycles)>
#
CCM  32k_4w_l1_I_CCM 0          32768 32 1
CCM  32k_4w_l1_D_CCM 1048576 32768 32 1

## Memory Management Unit definition (Android need MMU version 2)
#  MMU   <mmu name> <version>
MMU mmu_v1 1

## Core section (level 1)
#  CORE       <core name> <cpu clock ratio> <cpu data bus width (bits)>
#             <pipeline variant (EC5|EC7|SKIPJACK)> <warm-up period in cycles>
#
CORE          EC5_Castle32k_core 1 32 SKIPJACK 316
ADD_CACHE     32k_8w_l1_I I
ADD_CACHE     32k_4w_l1_D D
ADD_MMU       mmu_v1

# ADD CCMs to core
#
ADD_CCM     32k_4w_l1_I_CCM I
ADD_CCM     32k_4w_l1_D_CCM D

## Module section (level 2)
#  MODULE    <module name>
#
MODULE       EC5_Castle32k_module
ADD_CORE     EC5_Castle32k_core 1

## System section (level 3)
#  SYSTEM     <system name> <master clock freq (MHz)> <mem start address>
#             <mem end address> <mem data bus width (bits)>
#             <mem latency (cycles)> <mem bus clock divisor>
#
SYSTEM        EC5_Castle32k 250 0 4294967292 32 1 16
ADD_MODULE    EC5_Castle32k_module 1

##  ============================================================================
