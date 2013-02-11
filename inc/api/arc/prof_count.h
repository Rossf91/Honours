// =====================================================================
//
// SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
// Synopsys, Inc., and is fully protected under copyright and trade
// secret laws. You may not view, use, disclose, copy, or distribute
// this file or any information contained herein except pursuant to a
// valid written license from Synopsys.
//
// =====================================================================

#ifndef _prof_count_h_
#define _prof_count_h_

#define OEM_USE_OF_DEBUGGER_HEADER_FILES 1

#include "adrcount.h"
#include "api/types.h"


// Forward declare arcsim::util::Histogram
//
namespace arcsim {
  namespace util {
    class Histogram;
} }

typedef enum {
  PROF_COUNTER_ICNTS = 0,
  PROF_COUNTER_ICYCLES,
  PROF_COUNTER_LIMMS,
  PROF_COUNTER_CALLS,
  PROF_COUNTER_KILLEDS,
  PROF_COUNTER_DELAY_KILLEDS,
  PROF_COUNTER_ICACHE_MISS,
  PROF_COUNTER_ICACHE_MISS_CYCLES,
  PROF_COUNTER_DCACHE_MISS,
  PROF_COUNTER_DCACHE_MISS_CYCLES,
  N_PROFCNTS
} ProfCounter;

struct Prof_count : Addr_count {
private:
  const char* _label;
  const char* _id;
  const char* _name;
  const char* _short_name;
  bool&       _record;
  int         _enabled;
  arcsim::util::Histogram* _counter;

public:
  Prof_count(const char* id, const char* name, const char* short_name,
             const char* label,
             bool &record, int enabled);

  int version();
  const char *id();
  void destroy();
  int enabled();
  const char *name();
  const char *short_name();
  int get_count(unsigned adr, unsigned *same_as, unsigned *next);
  int next_address(int init, unsigned *addr);
  int clear();
  void init(uint32 sys_id);
};

#endif

