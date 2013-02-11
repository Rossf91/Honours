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

#include "api/mem/api_mem.h"

#include "sys/cpu/processor.h"
#include "sys/mem/Memory.h"

// -----------------------------------------------------------------------------
// Temporary macro for casting cpuContext and simContext into processor class.
// It provides better readability.
//
#define PROCESSOR(_cpu_) (reinterpret_cast<arcsim::sys::cpu::Processor*>(_cpu_))
#define SYSTEM(_sys_)    (reinterpret_cast<System*>(_sys_))


// ---------------------------------------------------------------------------
// Processor level read and write to memory
//
//DLLEXPORT int simCpuWriteWord (cpuContext ctx, uint32 addr, uint32 data);
int
simCpuWriteWord (cpuContext ctx, uint32 addr, uint32 data)
{
	return PROCESSOR(ctx)->write32 (addr, data);
}

//DLLEXPORT int simCpuWriteHalf (cpuContext ctx, uint32 addr, uint32 data);
int
simCpuWriteHalf (cpuContext ctx, uint32 addr, uint32 data)
{
	return PROCESSOR(ctx)->write16 (addr, data);
}

//DLLEXPORT int simCpuWriteByte (cpuContext ctx, uint32 addr, uint32 data);
int
simCpuWriteByte (cpuContext ctx, uint32 addr, uint32 data)
{
	return PROCESSOR(ctx)->write8 (addr, data);
}

//DLLEXPORT int simCpuReadWord (cpuContext ctx, uint32 addr, uint32* data);
int
simCpuReadWord (cpuContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(ctx)->read32 (addr, datum);
	if (status) 
    *data = datum;
	return status;
}

//DLLEXPORT int simCpuReadHalf (cpuContext ctx, uint32 addr, uint32* data);
int
simCpuReadHalf (cpuContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(ctx)->read16 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int simCpuReadByte (cpuContext ctx, uint32 addr, uint32* data);
int
simCpuReadByte (cpuContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(ctx)->read8 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int simCpuReadHalfSigned (cpuContext ctx, uint32 addr, uint32* data);
int
simCpuReadHalfSigned (cpuContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(ctx)->read16_extend (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int simCpuReadByteSigned (cpuContext ctx, uint32 addr, uint32* data);
int
simCpuReadByteSigned (cpuContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(ctx)->read8_extend (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int simCpuReadBlock (cpuContext ctx, uint32 addr, uint32 size, uint8 * buf);
int simCpuReadBlock (cpuContext     ctx,   /* processor context           */
                     uint32         addr,  /* physical start address      */
                     uint32         size,  /* read size in bytes          */
                     uint8 *        buf)   /* dest buffer of 'size' bytes */
{ 
  return PROCESSOR(ctx)->read_block(addr, size, buf);
}

// DLLEXPORT int simCpuWriteBlock (cpuContext ctx, uint32 addr, uint32 size, uint8 const * buf);
int simCpuWriteBlock (cpuContext     ctx,   /* processor context           */
                      uint32         addr,  /* physical start address      */
                      uint32         size,  /* read size in bytes          */
                      uint8 const *  buf)   /* src buffer of 'size' bytes  */
{ 
  return PROCESSOR(ctx)->write_block(addr, size, buf);
}

//DLLEXPORT int simCpuReadBlockExternalAgent (cpuContext ctx, uint32 addr, uint32 size, uint8 * buf, int id);
int simCpuReadBlockExternalAgent (
                     cpuContext     ctx,   /* processor context           */
                     uint32         addr,  /* physical start address      */
                     uint32         size,  /* read size in bytes          */
                     uint8 *        buf,   /* dest buffer of 'size' bytes */
                     int            agent_id)/* external agent id           */
{ 
  return PROCESSOR(ctx)->read_block_external_agent(addr, size, buf, agent_id);
}

// DLLEXPORT int simCpuWriteBlockExternalAgent (cpuContext ctx, uint32 addr, uint32 size, uint8 const * buf, int id);
int simCpuWriteBlockExternalAgent (
                      cpuContext    ctx,     /* processor context           */
                      uint32        addr,    /* physical start address      */
                      uint32        size,    /* read size in bytes          */
                      uint8 const * buf,      /* src buffer of 'size' bytes  */  
                      int           agent_id)/* external agent id          */
{ 
  return PROCESSOR(ctx)->write_block_external_agent(addr, size, buf, agent_id);
}

// ---------------------------------------------------------------------------
// System level read and write to memory
//
//DLLEXPORT int simWriteWord (simContext ctx, uint32 addr, uint32 data);
int
simWriteWord (simContext ctx, uint32 addr, uint32 data)
{
	return SYSTEM(ctx)->ext_mem->write32 (addr, data);
}

//DLLEXPORT int simWriteHalf (simContext ctx, uint32 addr, uint32 data);
int
simWriteHalf (simContext ctx, uint32 addr, uint32 data)
{
	return SYSTEM(ctx)->ext_mem->write16 (addr, data);
}

//DLLEXPORT int simWriteByte (simContext ctx, uint32 addr, uint32 data);
int
simWriteByte (simContext ctx, uint32 addr, uint32 data)
{
	return SYSTEM(ctx)->ext_mem->write8 (addr, data);
}

//DLLEXPORT int simReadWord (simContext ctx, uint32 addr, uint32* data);
int
simReadWord (simContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = SYSTEM(ctx)->ext_mem->read32 (addr, datum);
	if (status) 
    *data = datum;
	return status;
}

//DLLEXPORT int simReadHalf (simContext ctx, uint32 addr, uint32* data);
int
simReadHalf (simContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = SYSTEM(ctx)->ext_mem->read16 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int simReadByte (simContext ctx, uint32 addr, uint32* data);
int
simReadByte (simContext ctx, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = SYSTEM(ctx)->ext_mem->read8 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int simReadBlock (simContext ctx, uint32 addr, uint32 size, uint8 * buf);
int simReadBlock (simContext     ctx,   /* simulation context          */
                  uint32         addr,  /* physical start address      */
                  uint32         size,  /* read size in bytes          */
                  uint8 *        buf)   /* dest buffer of 'size' bytes */
{ 
  return SYSTEM(ctx)->ext_mem->read_block(addr, size, buf);
}


// DLLEXPORT int simWriteBlock (simContext ctx, uint32 addr, uint32 size, uint8 const * buf);
int simWriteBlock (simContext     ctx,   /* simulation context          */
                   uint32         addr,  /* physical start address      */
                   uint32         size,  /* read size in bytes          */
                   uint8 const *  buf)   /* src buffer of 'size' bytes  */
{ 
  return SYSTEM(ctx)->ext_mem->write_block(addr, size, buf);
}

//DLLEXPORT int simSimReadBlockExternalAgent (simContext ctx, uint32 addr, uint32 size, uint8 * buf, int id);
int simSimReadBlockExternalAgent(
                  simContext     ctx,   /* simulation context          */
                  uint32         addr,  /* physical start address      */
                  uint32         size,  /* read size in bytes          */
                  uint8 *        buf,   /* dest buffer of 'size' bytes */
                  int            agent_id)/* external agent id         */
{ 
  return SYSTEM(ctx)->ext_mem->read_block_external_agent(addr, size, buf, agent_id);
}


// DLLEXPORT int simSimWriteBlockExternalAgent (simContext ctx, uint32 addr, uint32 size, uint8 const * buf, int id);
int simSimWriteBlockExternalAgent (
                   simContext     ctx,   /* simulation context          */
                   uint32         addr,  /* physical start address      */
                   uint32         size,  /* read size in bytes          */
                   uint8 const *  buf,   /* src buffer of 'size' bytes  */
                   int            agent_id)/* external agent id         */
{ 
  return SYSTEM(ctx)->ext_mem->write_block_external_agent(addr, size, buf, agent_id);
}


