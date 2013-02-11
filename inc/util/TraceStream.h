//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Efficient, thread-safe TraceStream implementation. The processor class
// uses a TraceStream to output decoded/disassembled instructions.
//
// =====================================================================


#ifndef INC_UTIL_TRACESTREAM_H_
#define INC_UTIL_TRACESTREAM_H_

#include <cstdio>
#include <cstring>
#include <iosfwd>

namespace arcsim {
  namespace util {
    
    class TraceStream {
    private:
      // Internal buffer pointers
      //
      char *outBufStart, *outBufEnd, *outBufCur;
      
      // Filedescriptor used for flushing TraceStream
      //
      FILE*              fd_;
      
      void set_buffer(char *bufStart, size_t siz);
      void copy_to_buffer(const char* ptr, size_t siz);
      void flush_nonemtpy();
      
      TraceStream(const TraceStream &);     // DO NOT COPY
      void operator=(const TraceStream &);  // DO NOT ASSIGN
      
    public:
      // Constructor
      //
      explicit TraceStream(FILE* fd = stdout);
      
      // Destructor
      //
      ~TraceStream();
      
      void   set_out_fd(FILE* fd)         { fd_ = fd;                       }
      
      void   set_buffer_size(size_t siz)  { set_buffer(new char[siz], siz); }
      size_t get_buffer_size() const      { return outBufEnd - outBufStart; }
      
      TraceStream& write(const char* ptr, size_t siz);
      TraceStream& write(std::stringstream& S);
      
      void   flush();
      size_t flush(char *sink, size_t max_len);
      
    };
    
  } } //  arcsim::util

#endif // INC_UTIL_TRACESTREAM_H_
