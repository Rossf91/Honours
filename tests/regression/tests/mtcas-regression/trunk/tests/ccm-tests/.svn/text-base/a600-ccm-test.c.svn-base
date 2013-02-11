// COMPILE: arc-elf32-gcc -mA6 a600-ccm-test.c -o a600-ccm-test.x
// RUN: arcsim --fast --arch=ccm.arc --options=a600=1 --verbose --debug=2 --emt -e a600-ccm-test.x
// SIMULATED INSTRUCTIONS: 6980026577

#include <stdint.h>
#include <stdio.h>

#define DCCM_BASE_BUILD 0x61
#define DCCM_BUILD      0x74
#define AUX_ICCM_BUILD  0x078

//#define DO_DEBUG

#ifdef DO_DEBUG
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

struct DccmConfig {
  uint32_t start;
  uint32_t end;
  uint32_t size;
  uint32_t base_build_bcr;
  uint32_t build_bcr;
} DccmConfig;

struct IccmConfig {
  uint32_t start;
  uint32_t end;
  uint32_t size;
  uint32_t build_bcr;
};

static const uint32_t kLoopTimes = 0x8000;

int main (int argc, char const *argv[])
{
  struct DccmConfig dccm_config;
  struct IccmConfig iccm_config;
  
  // Read ICCM_BUILD BCR
  //
  __asm__ __volatile__ ("lr %0,[0x78]" : "=r"(iccm_config.build_bcr)      /* output - %0 */ : );
  
  if (iccm_config.build_bcr) {
    iccm_config.size = ((iccm_config.build_bcr >> 8) & 0x00000007UL);
    // If ICCM config size is NOT zero this indicates the presence of an ICCM
    //
    if (iccm_config.size) {
      DEBUG( printf("[ICCM] PRESENT\n"); )
      iccm_config.size = 0x2000 << (iccm_config.size - 1);
      DEBUG( printf("[ICCM] ICCM_BUILD:0x%08x\n", iccm_config.build_bcr); )
      DEBUG( printf("[ICCM] SIZE:%d bytes\n", iccm_config.size); )
      
      // Compute base address and size
      //
      iccm_config.start = iccm_config.build_bcr & 0xFFFFE000UL;
      iccm_config.end   = iccm_config.start + iccm_config.size;

      DEBUG( printf("[ICCM] BASE:0x%08x END:0x%08x\n", iccm_config.start, iccm_config.end); )
      
    }
  }
  
  // ---- DCCM ------------------------------------------------------------------------
  
  
  // Read DCCM_BUILD BCR first in order to determine presence of DCCM
  //
  __asm__ __volatile__ ("lr %0,[0x74]" : "=r"(dccm_config.build_bcr)      /* output - %0 */ : );
  
  if (dccm_config.build_bcr) { // non zero value indicates presence of DCCM
    DEBUG( printf("[DCCM] PRESENT\n"); )
    // Compute size
    //
    dccm_config.size = 0x800 << (dccm_config.build_bcr >> 8);
    DEBUG( printf("[DCCM] DCCM_BUILD:0x%08x\n", dccm_config.build_bcr); )
    DEBUG( printf("[DCCM] SIZE:%d bytes\n", dccm_config.size); )
    
    // Determine base address
    //
    __asm__ __volatile__ ("lr %0,[0x61]" : "=r"(dccm_config.base_build_bcr) /* output - %0 */ : );
    DEBUG( printf("[DCCM] DCCM_BASE_BUILD:0x%08x\n", dccm_config.base_build_bcr); )
    // Compute base address and size
    //
    dccm_config.start = dccm_config.base_build_bcr & 0xFFFFFF00;
    dccm_config.end   = dccm_config.start + dccm_config.size;
    
    DEBUG( printf("[DCCM] BASE:0x%08x END:0x%08x\n", dccm_config.start, dccm_config.end); )
    
    // RUN STUFF MANY TIMES SO IT WILL GET JIT COMPILED
    //
    uint32_t loop_times = kLoopTimes;
    while (--loop_times)
    { // Write some data to DCCM
      //
      uint32_t word_addr = dccm_config.start;
      uint32_t i = 0;
      for(i = 0; i < dccm_config.size; i+=4)
      {
        __asm__ __volatile__ ("st %0,[%1]" :
                                           : "r"(i),         /* input - %0 */ 
                                             "r"(word_addr)  /* input - %1 */ 
                                           );
       word_addr+=4;
      }
      
      // Make sure what we wrote to CCM got there correctly
      //
      word_addr = dccm_config.start;
      uint32_t word_data = 0;
      for(i = 0; i < dccm_config.size; i+=4)
      {
        __asm__ __volatile__ ("ld %1,[%0]" : "=r"(word_data)  /* output - %0 */ 
                                            : "r"(word_addr)  /* input  - %1 */ 
                                           );
       if (i != word_data) {
         DEBUG( printf("ERROR: DCCM reading bad data [expected:0x%08x] [read:0x%08x]\n", i, word_data); )
       }
       word_addr+=4;
      }
    }
  } else {
    DEBUG( printf("ERROR: DCCM not configured\n"); )
  }
  
  return 0;
}

