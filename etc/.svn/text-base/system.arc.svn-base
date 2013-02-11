##
## system.arc: Old System Architecture File 
##
## =============================================================================
##
## SYSTEM, MODULE and CORE sections:
## =================================
## - At least one SYSTEM, MODULE and CORE section must be defined. 
## - All sections and definitions must be defined in the following order:
##   CACHE     <cache name> <...>      # Cache definition 
##   CCM       <scratchpad name> <...> # CCM definition
##   BPU       <bpred name> <...>      # Branch predictor definition
##   CORE      <core name> <...>       # Start of a new Core section
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
##   CORE     <core name> <cpu clock ratio> <cpu data bus width (bits)>  <pipeline variant (EC5|EC7|ECPASTA)> <warm-up period in cycles>
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


# Cache definitions
CACHE    8KB_l1   8192 4 2 R 32 1 1
CACHE    16KB_l2 16384 4 2 R 32 10 2
CACHE    32KB_l2 32768 4 2 R 32 10 2
CACHE    64KB_l3 65536 4 2 R 32 100 4

# CCM definitions
CCM    8KB_ccm1 2147483648 8192 32 1
CCM    8KB_ccm2 2147491840 8192 32 1
CCM    16KB_ccm1 2147483648 16384 32 1
CCM    16KB_ccm2 2147500032 16384 32 1
CCM    test_ccm 0 1024000 32 1  

# BPU definitions
BPU    oracle-bp O 0 
BPU    gshare-bp A 1024 2 1024 5 5
BPU    gselect-bp E 1024 2 1024 5 5

# WPU definitions
WPU    32KB_wpu 4 8 4 32768 32 T    


# Core sections (level 1)
CORE         ARC750_core 1 32 EC5 316
ADD_CACHE    8KB_l1 I    
ADD_CACHE    8KB_l1 D
#ADD_CCM      8KB_ccm1 I 
#ADD_CCM      8KB_ccm2 D
ADD_BPU      gshare-bp
#ADD_BPU      gselect-bp
#ADD_BPU      oracle-bp
#ADD_WPU      32KB_wpu I
#ADD_WPU      32KB_wpu D

# Module sections (level 2)
MODULE       ARC750_module	
ADD_CORE     ARC750_core 1  
#ADD_CACHE    16KB_l2 U
#ADD_CACHE    32KB_l2 U
	
# System section (level 3)
SYSTEM        ARC750 533 0 4294967292 32 16 2
ADD_MODULE    ARC750_module 1 
#ADD_CACHE     64KB_l3 U


