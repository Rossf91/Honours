// BINARY: arc-elf32-gcc -mmul64  -mA6 mul64.c -o mul64.x
// RUNIT:  arcsim --verbose --emt --options=a600=1,mpy_option=mul64 --fast -e mul64.x
// INSTR. COUNT: 1000000222

int main (int argc, char const *argv[])
{
  unsigned int i = 0;
  unsigned int a = 1;
  unsigned int b = 2;
  unsigned int MLO  = 0;
  unsigned int MHI  = 0;
  unsigned int MMID = 0;
  
  // Test two long immediate multiplies
  //
  asm __volatile__ ("mulu64 0,0xFFFFFFFF,0xFFFFFFFF;\n\t"
                     "mov %0,r57;\n\t"
                     "mov %1,r58;\n\t"
                     "mov %2,r59;\n\t"
                     "sr %2,[0x12]"    /* write to AUX_MULHI */
                       : "=r"(MLO),    /* output %0 */
                         "=r"(MMID),   /* output %1 */
                         "=r"(MHI)     /* output %2 */
                       :
                       : "r57",        /* clobbered reg */
                         "r58",        /* clobbered reg */
                         "r59"         /* clobbered reg */
                     );
  
  
  // TEST MULU64
  //
  for(i = 0; i < 1000000; ++i)
  {
    a    = 1;
    b    = 2;
    MLO  = 0;
    MHI  = 0;
    MMID = 0;
    
    // Multiply (a * b) until we do not overflow MLO
    //
    while (MHI == 0)
    {
      asm __volatile__ ("mulu64 0,%3,%4;\n\t"
                        "mov %0,r57;\n\t"
                        "mov %1,r58;\n\t"
                        "mov %2,r59;\n\t"
                        "sr %2,[0x12]"    /* write to AUX_MULHI */
                          : "=r"(MLO),    /* output %0 */
                            "=r"(MMID),   /* output %1 */
                            "=r"(MHI)     /* output %2 */
                          : "r"(a),       /* input  %3 */
                            "r"(b)        /* input  %4 */
                          : "r57",        /* clobbered reg */
                            "r58",        /* clobbered reg */
                            "r59"         /* clobbered reg */
                        );
      // Store result from MLO into a
      //
      a = MLO;
    }
  }

  // TEST MUL64
  //
  for(i = 0; i < 1000000; ++i)
  {
    a    = 1;
    b    = 2;
    MLO  = 0;
    MHI  = 0;
    MMID = 0;
    
    // Multiply (a * b) until we do not overflow MLO
    //
    while (MHI == 0)
    {
      asm __volatile__ ("mul64 0,%3,%4;\n\t"
                        "mov %0,r57;\n\t"
                        "mov %1,r58;\n\t"
                        "mov %2,r59;\n\t"
                        "sr %2,[0x12]"    /* write to AUX_MULHI */
                          : "=r"(MLO),    /* output %0 */
                            "=r"(MMID),   /* output %1 */
                            "=r"(MHI)     /* output %2 */
                          : "r"(a),       /* input  %3 */
                            "r"(b)        /* input  %4 */
                          : "r57",        /* clobbered reg */
                            "r58",        /* clobbered reg */
                            "r59"         /* clobbered reg */
                        );
      // Store result from MLO into a
      //
      a = MLO;
    }
  }
  
  return (signed int)(MLO * argc);
}
