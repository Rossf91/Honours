// =====================================================================
//
// SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
// Synopsys, Inc., and is fully protected under copyright and trade
// secret laws. You may not view, use, disclose, copy, or distribute
// this file or any information contained herein except pursuant to a
// valid written license from Synopsys.
//
// =====================================================================

#define dbg 0

#define OEM_USE_OF_DEBUGGER_HEADER_FILES 1

#include "api/arc/prof_count.h"
#include "api/arc/api_arc_common.h"

#include "util/Histogram.h"

#include "ioc/Context.h"
#include "ioc/ContextItemInterface.h"

#if dbg
#include <iostream>
#include <iomanip>
#endif

using namespace arcsim::util;

Prof_count::Prof_count(const char* id, const char* name, const char* short_name, const char* label, bool &record, int enabled)
  : _id(id),
    _name(name),
    _short_name(short_name),
    _label(label),
    _record(record),
    _enabled(enabled)
{
}

int Prof_count::version() { return ADDR_COUNTINT_CLEAR_VERSION; }

const char *Prof_count::id() { return _id; }

void Prof_count::destroy() { 
  delete this;
}

int Prof_count::enabled() {
  int res =
    ((_record != 0) ? _enabled : 0) | 
    ADDRCOUNT_CALL_IF_DISABLED;
  return res; 
}

const char *Prof_count::name() { return _name; }

const char *Prof_count::short_name() { return _short_name ;}

int Prof_count::get_count(unsigned adr, unsigned *same_as, unsigned *next)
{
  int res = 0;
  if (_counter) {
    *same_as = adr & ~1;	// Even addresses only.
    *next = adr+2;
    if (_counter->index_exists(adr)) {
      res = _counter->get_value_at_index(adr);
    }
  }
  return res;
    
}

int Prof_count::next_address(int init, unsigned *addr) {
  static arcsim::util::HistogramIter * iter = 0;
  int res = 0;
  if (_counter) {
    if (init & 1) {
      if (iter) {
        delete iter;
        iter = 0;
      }
     iter = new arcsim::util::HistogramIter(*_counter);
    }
    if (iter) {
      while ( (!iter->is_end()) && ((**iter)->get_value()==0)) {
        ++(*iter);
      }

      if (!iter->is_end()) {
        *addr=(**iter)->get_index();
        ++(*iter);
        res = 1;
      }
    }
  }
  return res;
}

int Prof_count::clear() {
  if (_counter) {
    _counter->clear();
  }
  return 1;
}

void Prof_count::init(uint32 sys_id) {
  // Retrieve appropriate context that contains managed histograms
  //
  arcsim::ioc::Context& sys_ctx = *arcsim::ioc::Context::Global().get_context(sys_id);
  arcsim::ioc::Context& mod_ctx = *(sys_ctx.get_context(0));
  arcsim::ioc::Context& cpu_ctx = *(mod_ctx.get_context(0));
  
  // Retrieve appropriate ContextItem from cpu_ctx
  //
  _counter = static_cast<Histogram*>(cpu_ctx.get_item(_label));
  
  clear();
}

#define IMPNAME Prof_count
CHECK_ARC_OVERRIDES(Addr_count)


