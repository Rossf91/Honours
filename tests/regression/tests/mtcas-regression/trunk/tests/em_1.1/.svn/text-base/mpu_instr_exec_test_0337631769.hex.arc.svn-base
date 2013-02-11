# Cache definition:
CACHE ICACHE 2048 7 1 R 32 1 1
CACHE DCACHE 2048 7 4 R 32 1 1
# Closely Coupled Memory definition:


# Instruction Fetch Queue definition:

# MPU definition:
MMU MPU8 1 2 8
# Core section (level 1):
CORE arcv2em_core 1 32 SKIPJACK 316
# Add Caches to core:
ADD_CACHE ICACHE I
ADD_CACHE DCACHE D
# ADD CCMs to core:


# Add IFQ to core:

# Add MPU to core:
ADD_MMU MPU8
# Module section (level 2):
MODULE arcv2em_module
ADD_CORE arcv2em_core 1
# System section (level 3):
SYSTEM arcv2em_system 250 0 4294967292 32 1 16
ADD_MODULE arcv2em_module 1
