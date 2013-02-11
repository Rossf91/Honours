//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Memory subsystem API.
//
// =============================================================================


#ifndef INC_API_MEM_API_MEM_H_
#define INC_API_MEM_API_MEM_H_

#include "api/types.h"

#ifdef __cplusplus
extern "C" {
#endif

  // ---------------------------------------------------------------------------
  // Processor level read and write to memory
  //
  DLLEXPORT int  simCpuWriteWord (cpuContext cpu, uint32 addr, uint32  data);
  DLLEXPORT int  simCpuReadWord  (cpuContext cpu, uint32 addr, uint32* data);
  
  DLLEXPORT int  simCpuWriteHalf (cpuContext cpu, uint32 addr, uint32  data);
  DLLEXPORT int  simCpuReadHalf  (cpuContext cpu, uint32 addr, uint32* data);
  
  DLLEXPORT int  simCpuWriteByte (cpuContext cpu, uint32 addr, uint32  data);
  DLLEXPORT int  simCpuReadByte  (cpuContext cpu, uint32 addr, uint32* data);
  
  DLLEXPORT int  simCpuReadHalfSigned (cpuContext cpu, uint32 addr, uint32* data);
  DLLEXPORT int  simCpuReadByteSigned (cpuContext cpu, uint32 addr, uint32* data);

  // ---------------------------------------------------------------------------
  // Processor level block wise READ and WRITE to memory. Calls to these functions
  // will read and write memory and appear to be initiated by the simulator.
  //
  DLLEXPORT int simCpuReadBlock (cpuContext     ctx,   /* processor context           */
                                 uint32         addr,  /* physical start address      */
                                 uint32         size,  /* read size in bytes          */
                                 uint8 *        buf);  /* dest buffer of 'size' bytes */
  
  DLLEXPORT int simCpuWriteBlock (cpuContext    ctx,   /* processor context           */
                                  uint32        addr,  /* physical start address     */
                                  uint32        size,  /* write size in bytes        */
                                  uint8 const * buf);  /* src buffer of 'size' bytes */  
  

  // ---------------------------------------------------------------------------
  // Processor level block wise READ and WRITE to memory that should be used
  // by external agents. Calls to these functions will read and write memory
  // and appear to be initiated by an external agent with a given identifier.
  //
  DLLEXPORT int simCpuReadBlockExternalAgent (
                                 cpuContext     ctx,      /* processor context           */
                                 uint32         addr,     /* physical start address      */
                                 uint32         size,     /* read size in bytes          */
                                 uint8 *        buf,      /* dest buffer of 'size' bytes */
                                 int            agent_id);/* external agent id           */
  
  DLLEXPORT int simCpuWriteBlockExternalAgent (
                                  cpuContext    ctx,      /* processor context          */
                                  uint32        addr,     /* physical start address     */
                                  uint32        size,     /* write size in bytes        */
                                  uint8 const * buf,     /* src buffer of 'size' bytes  */  
                                  int           agent_id);/* external agent id          */
  
  // ---------------------------------------------------------------------------
  // System level read and write to memory
  //
  DLLEXPORT int  simWriteWord (simContext ctx, uint32 addr, uint32  data);
  DLLEXPORT int  simReadWord  (simContext ctx, uint32 addr, uint32* data);

  DLLEXPORT int  simWriteHalf (simContext ctx, uint32 addr, uint32  data);
  DLLEXPORT int  simReadHalf  (simContext ctx, uint32 addr, uint32* data);

  DLLEXPORT int  simWriteByte (simContext ctx, uint32 addr, uint32  data);
  DLLEXPORT int  simReadByte  (simContext ctx, uint32 addr, uint32* data);
  
  // ---------------------------------------------------------------------------
  // System level block wise READ and WRITE to memory
  //
  DLLEXPORT int simReadBlock (simContext     ctx,   /* simulation context          */
                              uint32         addr,  /* physical start address      */
                              uint32         size,  /* read size in bytes          */
                              uint8 *        buf);  /* dest buffer of 'size' bytes */

  DLLEXPORT int simWriteBlock (simContext    ctx,   /* simulation context         */
                               uint32        addr,  /* physical start address     */
                               uint32        size,  /* write size in bytes        */
                               uint8 const * buf);  /* src buffer of 'size' bytes */
  
  // ---------------------------------------------------------------------------
  // System level block wise READ and WRITE to memory that should be used
  // by external agents. Calls to these functions will read and write memory
  // and appear to be initiated by an external agent with a given identifier.
  //
  DLLEXPORT int simSimReadBlockExternalAgent (
                                              simContext     ctx,      /* simulation context           */
                                              uint32         addr,     /* physical start address      */
                                              uint32         size,     /* read size in bytes          */
                                              uint8 *        buf,      /* dest buffer of 'size' bytes */
                                              int            agent_id);/* external agent id           */
  
  DLLEXPORT int simSimWriteBlockExternalAgent (
                                               simContext    ctx,      /* simulation context          */
                                               uint32        addr,     /* physical start address     */
                                               uint32        size,     /* write size in bytes        */
                                               uint8 const * buf,     /* src buffer of 'size' bytes  */  
                                               int           agent_id);/* external agent id          */
  
#ifdef __cplusplus
}
#endif

#endif  // INC_API_MEM_API_MEM_H_
