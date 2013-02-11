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


#include "util/TraceStream.h"

#include "Assertion.h"

#include "util/OutputStream.h"
#include "concurrent/ScopedLock.h"

namespace arcsim {
  namespace util {

        TraceStream::TraceStream(FILE* fd) :
          outBufStart(0),
          outBufEnd(0),
          outBufCur(0),
          fd_(fd)
        { /* EMPTY */ }
        
        TraceStream::~TraceStream()
        {
          if (outBufStart != NULL) { delete [] outBufStart; }          
        }
        
        void
        TraceStream::set_buffer(char *bufStart, size_t siz)
        {
          ASSERT(bufStart && siz && "internal trace buffer must at least have one byte.");
          
          // Clear our previously allocated buffer
          //
          if (outBufStart != NULL) { delete [] outBufStart; }
          
          // Setup new pointers
          //
          outBufStart = bufStart;
          outBufEnd   = outBufStart + siz;
          outBufCur   = outBufStart;
          
          ASSERT(outBufStart <= outBufEnd && "invalid buffer size.");
        }
        
        // NOTE: This private method is NOT synchronised! Make sure you put
        //       proper synchronisation around it when using it!
        //
        void
        TraceStream::copy_to_buffer(const char *ptr, size_t siz)
        {          
          ASSERT(siz <= size_t(outBufEnd - outBufCur) && "internal buffer overrun!");
          
          // Special handling of short strings as memcpy isn't very efficient for
          // small sizes
          //
          switch (siz) {
            case 4: { outBufCur[3] = ptr[3]; }
            case 3: { outBufCur[2] = ptr[2]; } 
            case 2: { outBufCur[1] = ptr[1]; }
            case 1: { outBufCur[0] = ptr[0]; }
            case 0: break;
            default: {
              memcpy(outBufCur, ptr, siz);
              break;
            }
          }
          outBufCur += siz;
        }

        // NOTE: This private method is NOT synchronised! Make sure you put
        //       proper synchronisation around it when using it!
        //
        void
        TraceStream::flush_nonemtpy()
        {
          ASSERT(outBufCur > outBufStart && "trying to flush empty buffer!");
          size_t len = outBufCur - outBufStart;
          outBufCur = outBufStart;
          arcsim::util::OutputStream(fd_).Get().write(outBufStart, len);
        }

        
        TraceStream&
        TraceStream::write(const char *ptr, size_t siz)
        {
          if ( (outBufCur + siz) > outBufEnd) {
            // We need to flush data in the buffer
            //
            do {
              size_t numBytes = outBufEnd - outBufCur;
              copy_to_buffer(ptr, numBytes);
              flush_nonemtpy();
              ptr += numBytes;
              siz -= numBytes;
            } while ( (outBufCur + siz) > outBufEnd);
          }
          
          copy_to_buffer(ptr, siz);

          return *this;
        }
        
        TraceStream&
        TraceStream::write(std::stringstream& S) {
          if (S.seekg(0, std::ios::end).tellg() > 0) {
            // Only if stringstream is not empty do we write something
            //
            write(S.str().c_str(), S.tellg());
            
            // Put internal pointer back to the beginning so there are no unpleasant
            // surprises for the callers of this method
            //
            S.seekg(0, std::ios::beg);
          }
          return *this;
        }

        
        void
        TraceStream::flush()
        {
          if (outBufCur != outBufStart) {
            flush_nonemtpy();
          }
        }

        size_t
        TraceStream::flush(char *sink, size_t max_len)
        {
          ASSERT(outBufCur > outBufStart && "trying to flush empty buffer!");
          size_t len = outBufCur - outBufStart;
          len = (len > max_len) ? max_len : len;  // May truncate if too long
          strncpy(sink, outBufStart, len);
          outBufCur = outBufStart;
          return len;
        }

} } //  arcsim::util
