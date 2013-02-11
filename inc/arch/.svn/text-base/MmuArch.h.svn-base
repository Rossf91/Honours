//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a MMU (Memory Management Unit) configuration.
//
// =====================================================================

#ifndef INC_ARCH_MMUARCH_H_
#define INC_ARCH_MMUARCH_H_

// define page size parameter entry
// param -> (str,size,log2(size),bcr encoding)
#define MMU_PAGE_SIZE_LIST(V)  \
  V(1K,1024,10,1)              \
  V(2K,2048,11,2)              \
  V(4K,4096,12,3)              \
  V(8K,8192,13,4)              \
  V(16K,16384,14,5)

// define supported MMU versions
#define MMU_VERSION_LIST(V) \
  V(1)                      \
  V(2)                      \
  V(3)

// define supported JTLB set sizes
#define MMU_JTLB_SET_SIZE_LIST(V) \
  V(128)

// define supported JTLB way sizes
#define MMU_JTLB_WAY_SIZE_LIST(V) \
  V(2)                            \
  V(4)

// Forward declare MMU PageSizeParamEntry
struct PageSizeParamEntry;

class MmuArch
{
public:
  static const int kMmuArchMaxNameSize = 256;
  
  // supported MMU versions
  enum Version {
#define DECL_ENUM_V(version_) kMmuV##version_ = version_,  
    MMU_VERSION_LIST(DECL_ENUM_V)
#undef DECL_ENUM_V
  };

  enum Kind{
    kMmu = 0,
    kMpu = 1
  };

  bool    is_configured;
  char    name[kMmuArchMaxNameSize];
  
  uint32  version;        // MMU version identifier
  
  uint32  u_itlb_entries; // ITLB entries
  uint32  u_dtlb_entries; // DTLB entries

  uint32   mpu_num_regions;

  uint32 kind;
  
  MmuArch();
  ~MmuArch();

  // NOTE: set_page_size() returns 'false' and does not change the default page
  //       size if invalid size has been specified.
  bool    set_page_size(uint32 size_in_bytes);  
  uint32  get_page_size() const;
  uint32  get_page_size_log2() const;
  uint32  get_page_size_bcr_encoding() const;
  
  // NOTE: A700 has 0x7 (2^7=128) JTLB sets as a default, specifying illegal size
  //       does not change default
  bool    set_jtlb_sets(uint32 sets);
  uint32  get_jtlb_sets() const;
  uint32  get_jtlb_sets_log2() const;

  // NOTE: A700 has 0x1 (2^1=2) JTLB ways as a default, MMUv3 supports 2-way and
  //       4-way configurations, specifying illegal size does not change default
  bool    set_jtlb_ways(uint32 ways);
  uint32  get_jtlb_ways() const;
  uint32  get_jtlb_ways_log2() const;

  uint32  get_jtlb_entries() const;
  
private:
  PageSizeParamEntry* entry_;
  uint32  jtlb_ways_;
  uint32  jtlb_sets_;
  
};

#endif  // INC_ARCH_MMUARCH_H_
