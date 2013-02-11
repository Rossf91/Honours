EEMBC 1.1 Bare Metal Binaries: Configuration

Author: igor.boehm@ed.ac.uk
Date:   13.04.2010
--------------------------------------------------------------------------------

These benchmarks have been configured to run over 1.000.000.000 instructions
so we can properly test our JIT DBT engine.


NOTE that CRC checks have been disabled.

// Compiler Configuration
////
Compiler: arc-elf32-gcc 4.2.1 (ARC_2.0_sourceforge)
Configured with:
  ../src/configure --target=arc-elf32 --with-newlib --with-headers
                   --enable-multilib --enable-languages=c

// Pre-Processor/Compiler/Assembler/Linker Flags
////
CPPFLAGS+=  -I$(TOP)/inc -I${CC_ARC_BASE}/arc-elf32/include
CFLAGS+=    -S -O3 -Wall -mA7
ASFLAGS+=   -mA7 -mEA
LDFLAGS+=   -x -Bstatic -marcelf -T ${LIB_ENCORE}/link.bare.ld


This set of benchmarks does not represent the full range of EEMBC 1.1 since
there are some where the compiler produces bad code with -O3 (e.g. non-aligned
memory access), the following benchmark havs been compiled with '-O2':

- automotive/matrix01   - produced bad code with -O3 (non-aligned memory access)

