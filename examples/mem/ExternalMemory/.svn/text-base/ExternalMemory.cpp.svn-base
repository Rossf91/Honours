//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#include <iomanip>

#include "mem/MemoryDeviceInterface.h"

#include "util/Log.h"

// -----------------------------------------------------------------------------
// Sample MemoryDevice device
//
class ExternalMemoryDevice : public arcsim::mem::MemoryDeviceInterface
{
private:
  uint32 *data32;
	uint16 *data16;
	uint8  *data8;
	uint32 words;
public:
  // MemoryDevice base address
  // NOTE this must align to 8K boundaries at the moment (this restriction will soon be removed)
  //
  static const uint32 kExternalMemoryBaseAddr = 0x00000000;
  // MemoryDevice memory size
  // NOTE this must by multiple of 8K at the moment (this restriction will soon be removed)
  //
  static const uint32 kExternalMemoryByteSize = 0x00008000;
  
  // ---------------------------------------------------------------------
  // Constructor
  //
  ExternalMemoryDevice()
  {
    words   = kExternalMemoryByteSize >> 2;
    data32  = new uint32 [words];
    data16  = (uint16 *)data32;
    data8   = (uint8  *)data32;
  }
  // ---------------------------------------------------------------------
  // Destructor
  //
  ~ExternalMemoryDevice()
  {
    delete [] data32;
  }
  // ---------------------------------------------------------------------
  // Query range for which this MemoryDevice is responsible
  //
  uint32 get_range_begin()      const { return kExternalMemoryBaseAddr;                           }
  uint32 get_range_end  ()      const { return kExternalMemoryBaseAddr + kExternalMemoryByteSize; }
  
  // ---------------------------------------------------------------------
  // Initialise memory device
  //
  int
  mem_dev_init  (uint32 val)
  {
    for (uint32 i = 0; i < words; ++i) { data32[i] = val; }
    return IO_API_OK;
  }
  
  // ---------------------------------------------------------------------
  // Clear memory device
  //
  int
  mem_dev_clear  (uint32 val)
  {
    for (uint32 i = 0; i < words; ++i) { data32[i] = val; }
    return IO_API_OK;
  }
  
  // ---------------------------------------------------------------------
  // Perform READ transaction
  //
  int
  mem_dev_read  (uint32 addr, unsigned char* dest, int size)
  { 
    bool    success = true;
    uint32  data    = 0;
    // Perform READ operation
    //
    switch (size) {
      case 1: { // READ BYTE
        *((uint32*)(dest)) = data8 [addr - kExternalMemoryBaseAddr];
        break;
      }
      case 2: { // READ HALF WORD (2 Bytes)
        *((uint32*)(dest)) = data16[(addr - kExternalMemoryBaseAddr) >> 1];
        break;
      }
      case 4: { // READ WORD  (4 Bytes)
        *((uint32*)(dest)) = data32[(addr - kExternalMemoryBaseAddr) >> 2];
        break;
      }
      default: {
        LOG(LOG_ERROR) << "[EXTMEM] mem_dev_read: illegal write size '" << size << "'";
        success = false;
        break;
      }
    }
    LOG(LOG_DEBUG) << "[EXTMEM] mem_dev_read addr: '0x"
                   << std::hex << std::setw(8) << std::setfill('0') << addr
                   << "' data:'0x"
                   << std::hex << std::setw(8) << std::setfill('0') << *((uint32*)(dest))
                   << "' size:"
                   << std::dec << size;

    return (success) ? IO_API_OK : IO_API_ERROR;
  }
  // ---------------------------------------------------------------------
  // Perform WRITE transaction
  //
  int
  mem_dev_write (uint32 addr, const unsigned char *data, int size)
  {
    bool success = true;
    // Perform READ operation
    //
    switch (size) {
      case 1: {  // WRITE BYTE
        data8 [addr - kExternalMemoryBaseAddr]        = *((uint8*) (data));
        LOG(LOG_DEBUG) << "[EXTMEM] mem_dev_write addr: '0x"
                       << std::hex << std::setw(8) << std::setfill('0') << addr
                       << "' data:'0x"
                       << std::hex << std::setw(8) << std::setfill('0')
                       << (uint32)(*((uint8*)(data)))
                       << "' size:"
                       << std::dec << size;
        break;
      }
      case 2: { // WRITE HALF WORD (2 Bytes)
        data16[(addr - kExternalMemoryBaseAddr) >> 1] = *((uint16*)(data));
        LOG(LOG_DEBUG) << "[EXTMEM] mem_dev_write addr: '0x"
                       << std::hex << std::setw(8) << std::setfill('0') << addr
                       << "' data:'0x"
                       << std::hex << std::setw(8) << std::setfill('0')
                       << (uint32)(*((uint16*)(data)))
                       << "' size:"
                       << std::dec << size;
        break;
      }
      case 4: { // WRITE WORD (4 Bytes)
        data32[(addr - kExternalMemoryBaseAddr) >> 2] = *((uint32*)(data));
        LOG(LOG_DEBUG) << "[EXTMEM] mem_dev_write addr: '0x"
                       << std::hex << std::setw(8) << std::setfill('0') << addr
                       << "' data:'0x"
                       << std::hex << std::setw(8) << std::setfill('0')
                       << (uint32)(*((uint32*)(data)))
                       << "' size:"
                       << std::dec << size;
        break;
      }
      default: {
        LOG(LOG_ERROR) << "[EXTMEM] mem_dev_write: illegal write size '" << size << "'";
        success = false;
        break;
      }
    }
    return (success) ? IO_API_OK : IO_API_ERROR;
  }
  
  // -----------------------------------------------------------------------
  // Debug read/write implementations
  //
  int
  mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
  {
    return mem_dev_read(addr, dest, size);
  }
  
  int
  mem_dev_write (uint32 addr, const unsigned char*  data, int size, int agent_id)
  {
    return mem_dev_write(addr, data, size);
  }

};
      
// -----------------------------------------------------------------------------
// Function called by simulator giving control to MemoryDevice library so
// that it can register as many MemoryDevices as it desires.
//
void simLoadMemoryDevice (simContext ctx)
{
  simRegisterMemoryDevice(ctx, new ExternalMemoryDevice());
}

