//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines macros used to interpret trap instructions as
//  linux system calls made via uclibc i/o functions. This is intended
//  as a temporary solution for the requirement to test applications
//  in the absence of a full system simulation. Most traps are not yet
//  implemented.
//
// =====================================================================

#include <termios.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <cstring>
#include <ctime>

#include "api/types.h"
#include "trap.h"
/* syscall numbers taken from asm-arc/unistd.h */

#include "arc-unistd.h"

#include "system.h"

#include "sys/cpu/processor.h"

#include "util/Log.h"

// ARC-Linux system call conventions:
//  
//   Syscall  Syscall  <---Argument registers-->    Result
//    kind    number   0     1     2     3     4   register
// -------------------------------------------------------------
//   syscall0    r8    -     -     -     -     -      r0
//   syscall1    r8   r0     -     -     -     -      r0
//   syscall2    r8   r0    r1     -     -     -      r0
//   syscall3    r8   r0    r1    r2     -     -      r0
//   syscall4    r8   r0    r1    r2    r3     -      r0
//   syscall5    r8   r0    r1    r2    r3    r4      r0
// -------------------------------------------------------------
//
// Each syscall is invoked with a 'trap0' instruction and after
// each trap0 returns to the calling syscall sequence, the following
// return sequence is executed:
//
//  if ((uint32)(__res) >= (uint32)(-125)) {
//    errno = -__res;
//    __res = -1;
//  }
//  return (type)__res;
//
// In this sequence, __res is the value returned in the result register.
//
// The following inline syscalls are defined in ARC-Linux:
//
//   Syscall       Syscall  Return   Argument list
//    name           kind    type     type and name
// ---------------------------------------------------------------------------
//   pause         syscall0  int       -
//   sync          syscall0  int       -
//   setsid        syscall0  pid_t     -
//   close         syscall1  int       int fd
//   dup           syscall1  int       int fd
//   _exit         syscall1  int       int exitcode
//   delete_module syscall1  int       const char* name
//   write         syscall3  int       int fd, const char* buf, off_t count
//   read          syscall3  int       int fd char* buf, off_t count
//   lseek         syscall3  off_t     int fd, off_t offset, int count
//   open          syscall3  int       const char* file, int flag, int mode
//   waitpid       syscall3  pid_t     pid_t pid, int* wait_stat, int options
//   ioctl         syscall3  int       int fd, int request, char* argp
// ---------------------------------------------------------------------------
//
// Initially only a small number of syscalls are handled by the simulator,
// but the full set of system call numbers is defined in Linux asm-arc/unistd.h


//#define DEBUG_IOCTL


// ---------------------------------------------------------------------------
// Byte wise memory write function used mainly by trap emulation to write data
// into simulator memory.
//
static inline bool
trap_write_block (arcsim::sys::cpu::Processor&  cpu,
                  char*                         buf,
                  uint32                        addr,
                  int                           size)
{
  if (size < 0) return true;
  
  bool   success = true;
  
  for (uint32 i = 0; (i < size) && success; ++i) {
    success = cpu.write8(addr+i, (uint32)buf[i]);
  }
  
  return success;
}

// ---------------------------------------------------------------------------
// Byte wise memory read function used mainly by trap emulation to read data
// from simulator memory.
//
static inline bool
trap_read_block(arcsim::sys::cpu::Processor& cpu,
                char *                       buf,
                uint32                       addr,
                int                          size)
{
  if (size < 0) return true;
  
  bool   success = true;        
  uint32 rdata   = 0;
  
  for (uint32 i = 0; (i < size) && success; ++i) {
    success = cpu.read8(addr+i, rdata);
    buf[i]  = (char)rdata;
  }
  return success;
}


// ---------------------------------------------------------------------------
// System call emulation
//
void System::emulate_syscall (arcsim::sys::cpu::Processor&  _cpu,
                              int                           call, 
                              uint32&                       res, 
                              uint32                        a0, 
                              uint32                        a1, 
                              uint32                        a2, 
                              uint32                        a3, 
                              uint32                        a4)
{	
	switch (call) {
      
    case __ARC_LINUX_NR_read:
		{
			char* rd_buf = new char [a2];
			if (sim_opts.redir_std_input && (a0 == STDIN_FILENO)) { 
				res = read (sim_opts.rin_fd, (void*)rd_buf, a2);
			} else {
				res = read (a0, (void*)rd_buf, a2);
      }
			if (res == (uint32)-1) {
        LOG(LOG_DEBUG) << "[CPU" << _cpu.core_id << "] SYSCALL: READ failed - "
        << strerror(errno);
				res = -errno;
      }
      // Write read value to simulation memory
      //
			trap_write_block (_cpu, rd_buf, a1, a2);
			delete [] rd_buf;
		}
      break;
      
    case __ARC_LINUX_NR_write:
		{
			char* wr_buf = new char [a2];
      
      // Read from simulation memory
      //
			trap_read_block (_cpu, wr_buf, a1, a2);
      
      // Write values to host memory
      //
      if (sim_opts.redir_std_output && (a0 == STDOUT_FILENO)) {
				res = write (sim_opts.rout_fd, (void*)wr_buf, a2);			
			} else if (sim_opts.redir_std_error && (a0 == STDERR_FILENO)) {
				res = write (sim_opts.rerr_fd, (void*)wr_buf, a2);			
			} else {
				res = write (a0, (void*)wr_buf, a2);			
			}
			if (res == (uint32)-1) {
        LOG(LOG_DEBUG) << "[CPU" << _cpu.core_id << "] SYSCALL: WRITE failed - "
        << strerror(errno);
				res = -errno;
      }
      
			delete [] wr_buf;
		}
      break;
      
    case __ARC_LINUX_NR_readv:
    {
      /* Arguments: int fd [a0], const struct iovec* vector [a1], int count [a2] */
      struct iovec host_vectors[a2];
      struct __arc_iovec arc_vectors[a2];
      
      trap_read_block (_cpu, (char *)arc_vectors, a1, (uint32)sizeof(struct __arc_iovec) * a2);
      
      // Allocate host_vectors of same size as described by arc_vectors
      for (uint32 i=0; i < a2; i++) {
        host_vectors[i].iov_base = new char[arc_vectors[i].iov_len];
        host_vectors[i].iov_len = arc_vectors[i].iov_len;
      }
      
      if (sim_opts.redir_std_input && (a0 == STDIN_FILENO)) {
        a0 = sim_opts.rin_fd;
      }
      
      res = (uint32)readv(a0, host_vectors, a2);
      
      if (res == (uint32)-1) {
        res = -errno;
        for (uint32 i=0; i < a2; i++)
          delete[] (char *)host_vectors[i].iov_base;
      } else {
        // Write the blocks to simulated memory, as described by arc_vectors
        for (uint32 i=0; i < a2; i++) {
          trap_write_block (_cpu, (char *)host_vectors[i].iov_base, arc_vectors[i].iov_base, arc_vectors[i].iov_len);
          delete[] (char *)host_vectors[i].iov_base;
        }
      }
    }
      break;
      
    case __ARC_LINUX_NR_writev:
    { 
      /* Arguments: int fd [a0], const struct iovec* vector [a1], int count [a2] */
      struct iovec host_vectors[a2];
      struct __arc_iovec arc_vectors[a2];
      
      trap_read_block (_cpu, (char *)arc_vectors, a1, (uint32)sizeof(struct __arc_iovec) * a2);
      
      // Allocate host_vectors of same size as described by arc_vectors and copy contents
      for (uint32 i=0; i < a2; i++) {
        host_vectors[i].iov_base = new char[arc_vectors[i].iov_len];
        host_vectors[i].iov_len = arc_vectors[i].iov_len;
        trap_read_block (_cpu, (char *)host_vectors[i].iov_base, arc_vectors[i].iov_base, arc_vectors[i].iov_len);
      }
      
      if (sim_opts.redir_std_output && (a0 == STDOUT_FILENO)) {
        res = (uint32)writev(sim_opts.rout_fd, host_vectors, a2);
			} else if (sim_opts.redir_std_error && (a0 == STDERR_FILENO)) {
				res = (uint32)writev(sim_opts.rerr_fd, host_vectors, a2);			
			} else {
				fflush(stderr);
				fflush(stdout);
				res = (uint32)writev(a0, host_vectors, a2);			
				fflush(stderr);
				fflush(stdout);
			}
      
      if (res == (uint32)-1)
        res = -errno;
      
      for (uint32 i=0; i < a2; i++)
        delete[] (char *)host_vectors[i].iov_base;      
    }
      break;
      
    case __ARC_LINUX_NR_open:
		{
			char fname [256];
			trap_read_block (_cpu, fname, a0, 256);
			
      res = open (fname, a1, a2);
    
      if (res == (uint32)-1) {
        LOG(LOG_DEBUG) << "[CPU" << _cpu.core_id << "] SYSCALL: OPEN '" << fname << "' failed - "
                       << strerror(errno);
        res = -errno;
      }
		}
      break;
      
    case __ARC_LINUX_NR_close:
		{
      // Do not close stdin/stdout/stderr of simulation host
      //
      if (a0 != STDERR_FILENO && a0 != STDOUT_FILENO && a0 != STDIN_FILENO) {
        res = close (a0);
        if (res == (uint32)-1) {
          LOG(LOG_DEBUG) << "[CPU" << _cpu.core_id << "] SYSCALL: CLOSE failed - "
                         << strerror(errno);
          res = -errno;
        }
      } else {
        // If the simulated application is trying to close stdin/stdout/stderr
        // on the simulation host we pretend it went fine.
        //
        res = 0;
      }
		}
      break;
      
    case __ARC_LINUX_NR_unlink:
		{
			char fname [256];
			trap_read_block (_cpu, fname, a0, 256);
      
			res = unlink(fname);
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_ioctl:
		{
			unsigned io_size  = _IOC_SIZE(a2);
			unsigned io_dir   = _IOC_DIR(a2);
			char* syscall_buf = new char [io_size];
#ifdef DEBUG_IOCTL
			fprintf (stderr, "__ARC_LINUX_NR_ioctl: a0 = %08x, a1 = %08x, a2 = %08x\n", a0, a1, a2);
#endif		
			if (io_dir == _IOC_WRITE) {
				/* copy simulator argument space into host buffer prior to host ioctl call */
				trap_read_block (_cpu, syscall_buf, a2, io_size);
#ifdef DEBUG_IOCTL
				fprintf (stderr, "__ARC_LINUX_NR_ioctl: _IOC_WRITE buf => '");
				for (int i = 0; i < io_size; i++) fprintf (stderr, "%c", syscall_buf[i]);
#endif		
			}
      
#if (defined(__APPLE__) && defined(__MACH__))
      // FIXME: mappings of ioctl requests on Darwin/BSD are different:
      // @see: http://book.opensourceproject.org.cn/kernel/linux011/opensource/termios_8h-source.html
      // @see: http://www.gnu-darwin.org/www001/src/ports/lang/fpc-devel/work/fpc/rtl/linux/termiosproc.inc.html
      //
      switch (a1) {
        case 0x5401: // TCGETS
          ioctl (a0, TIOCGETA, syscall_buf); break;
        case 0x5402: // TCSETS
          ioctl (a0, TIOCSETA, syscall_buf); break;
          // FIXME: there are more mappings that we do not deal with...
          //
        default:
          ioctl (a0, a1, syscall_buf); break;
      }
#else
      ioctl (a0, a1, syscall_buf);
#endif
			res = -errno;
      
#ifdef DEBUG_IOCTL
			fprintf (stderr, "__ARC_LINUX_NR_ioctl: res = %08x, errno = %d\n", res, errno);
#endif
      
			if (io_dir == _IOC_READ) {
				/* copy result from host buffer to both model's memories */
#ifdef DEBUG_IOCTL
				fprintf (stderr, "__ARC_LINUX_NR_ioctl: _IOC_READ buf <= '");
				for (int i = 0; i < io_size; i++) fprintf (stderr, "%c", syscall_buf[i]);
#endif		
				trap_write_block (_cpu, syscall_buf, a2, io_size);
			}
			delete [] syscall_buf;
		}
      break;
      
    case __ARC_LINUX_NR_stat:
    case __ARC_LINUX_NR_lstat:
		{
			struct stat st;
      struct __arc_kernel_stat result_st;
      
			char fname [256];
			trap_read_block (_cpu, fname, a0, 256);
      
			
      if (call == __ARC_LINUX_NR_stat) 
				res = stat (fname, &st);
			else
				res = lstat (fname, &st);
      
      result_st.arcsim_dev   = st.st_dev;
      result_st.arcsim_ino   = (uint32)st.st_ino;
      result_st.arcsim_mode  = st.st_mode;
      result_st.arcsim_nlink = st.st_nlink;
      result_st.arcsim_uid   = st.st_uid;
      result_st.arcsim_gid   = st.st_gid;
      result_st.arcsim_rdev  = st.st_rdev;
      result_st.arcsim_size  = (uint32)st.st_size;
      result_st.arcsim_atime = (uint32)st.st_atime;
      result_st.arcsim_mtime = (uint32)st.st_mtime;
      result_st.arcsim_ctime = (uint32)st.st_ctime;
      
      trap_write_block (_cpu, (char*)&result_st, a1, sizeof(struct __arc_kernel_stat));
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_stat64:
		{
      struct stat64 st;
      struct __arc_kernel_stat64 result_st;
      
			char fname [256];
			trap_read_block (_cpu, fname, a0, 256);
      
   	  
      res = (uint32)stat64 (fname, &st);
      
      result_st.arcsim_dev   = st.st_dev;
      result_st.arcsim_ino   = st.st_ino;
      result_st.__arcsim_ino = (uint32)st.st_ino;
      result_st.arcsim_mode  = st.st_mode;
      result_st.arcsim_nlink = st.st_nlink;
      result_st.arcsim_uid   = st.st_uid;
      result_st.arcsim_gid   = st.st_gid;
      result_st.arcsim_rdev  = st.st_rdev;
      result_st.arcsim_size  = st.st_size;
      result_st.arcsim_atime = (uint32)st.st_atime;
      result_st.arcsim_mtime = (uint32)st.st_mtime;
      result_st.arcsim_ctime = (uint32)st.st_ctime;
      
			trap_write_block (_cpu, (char*)&result_st, a1, sizeof(struct __arc_kernel_stat64));
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_fstat:
		{
			struct stat st;
      struct __arc_kernel_stat result_st;
      
			res = fstat (a0, &st);
      
      result_st.arcsim_dev   = st.st_dev;
      result_st.arcsim_ino   = (uint32)st.st_ino;
      result_st.arcsim_mode  = st.st_mode;
      result_st.arcsim_nlink = st.st_nlink;
      result_st.arcsim_uid   = st.st_uid;
      result_st.arcsim_gid   = st.st_gid;
      result_st.arcsim_rdev  = st.st_rdev;
      result_st.arcsim_size  = (uint32)st.st_size;
      result_st.arcsim_atime = (uint32)st.st_atime;
      result_st.arcsim_mtime = (uint32)st.st_mtime;
      result_st.arcsim_ctime = (uint32)st.st_ctime;
      
			trap_write_block (_cpu, (char*)&result_st, a1, sizeof(struct __arc_kernel_stat));
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_rmdir:
		{
			char fname [256];
			trap_read_block (_cpu, fname, a0, 256);
      
      
      res = rmdir(fname);
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_lseek:
		{
			res = (uint32)lseek (a0, a1, a2);
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR__llseek:
		{ 
      /* Arguments: unsigned int fd [a0], unsigned long offset_high [a1],
       unsigned long offset_low [a2], loff_t (=64bit always) *result [a3],
       unsigned int whence [a4] */
			loff_t loff_t_result;
      
#if (defined(__APPLE__) && defined(__MACH__))
      loff_t_result = lseek(a0, ((uint64)a1 << 32) | a2, a4);
#else
      loff_t_result = lseek64(a0, ((uint64)a1 << 32) | a2, a4);
#endif
      
			if (loff_t_result == (loff_t)-1) {
				res = -errno;
      } else {
        res = 0;
      }
      
      uint32 resultH, resultL;
			resultL = (uint32)(loff_t_result & 0xFFFFFFFF);
			resultH = (uint32)(loff_t_result >> 32);
			_cpu.write32(a3, resultL);
			_cpu.write32(a3+4, resultH);
		}
      break;
      
    case __ARC_LINUX_NR_getrusage:
		{
			struct rusage ru;
			res = getrusage (a0, &ru);		
			trap_write_block (_cpu, (char*)&ru, a1, sizeof(struct rusage));
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_setrlimit:
		{
			struct rlimit rl;
			
			_cpu.read32(a1, (uint32&) rl.rlim_cur);
			_cpu.read32(a1+4, (uint32&) rl.rlim_max);
      
			res = setrlimit (a0, &rl);		
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_getrlimit:
    case __ARC_LINUX_NR_old_getrlimit:
		{
			struct rlimit rl;
			
			res = getrlimit (a0, &rl);		
			if (res == (uint32)-1)
				res = -errno;
      
			_cpu.write32(a1, (uint32)rl.rlim_cur);
			_cpu.write32(a1+4, (uint32)rl.rlim_max);
		}
      break;
      
    case __ARC_LINUX_NR_mmap:
		{
#if 0
			// 32-bit align mapping
			if (heap_limit & 0x03)
				res = (heap_limit + 4) & 0xFFFFFFFC;
			else
				res = heap_limit;
      
			heap_limit = res + a1;
#else
			res = heap_limit_;
			heap_limit_ += a1;
#endif
		}
      break;
      
    case __ARC_LINUX_NR_mremap:
		{
			if (a2 > a1) {
				if (heap_limit_ & 0x03)
					res = (heap_limit_ + 4) & 0xFFFFFFFC;
				else
					res = heap_limit_;
        
				for (uint32 bytes=0; bytes < a1; bytes++) {
					uint32 data = 0;
					_cpu.read8(a0+bytes, data);
					_cpu.write8(res+bytes, data);
				}
        
				heap_limit_ = res + a2;
        
			} else 
				res = a0;
		}
      break;
      
    case __ARC_LINUX_NR_munmap:
		{
      
			res = 0;
		}
      break;
      
    case __ARC_LINUX_NR_brk:
		{
      
			/* brk always succeeds - memory is unlimited */
			res = 0;
		}
      break;
      
    case __ARC_LINUX_NR_times:
		{
      
			struct tms host_tbuf;
      struct __arc_tms arc_tbuf;
			
			res = (uint32) times(&host_tbuf);
      
      arc_tbuf.tms_utime = (uint32)host_tbuf.tms_utime;
      arc_tbuf.tms_stime = (uint32)host_tbuf.tms_stime;
      arc_tbuf.tms_cutime = (uint32)host_tbuf.tms_cutime;
      arc_tbuf.tms_cstime = (uint32)host_tbuf.tms_cstime;
      
			trap_write_block (_cpu, (char*)(&arc_tbuf), a0, sizeof(struct __arc_tms));
      
			if (res == (uint32)-1)
				res = -errno;
		}
      break;
      
    case __ARC_LINUX_NR_gettimeofday:
    {
      struct timeval host_tv;      
#ifdef __LP64__     
      // On 64-bit simulation host
      //
      struct __arc_timeval arc_tv;
      
      // FIXME: Add timezone support
      //
      
      // Actually get timeofday
      //
      res = gettimeofday (&host_tv, (struct timezone *)NULL);
      
      arc_tv.tv_sec   = (uint32)host_tv.tv_sec;
      arc_tv.tv_usec  = (uint32)host_tv.tv_usec;
      
      // Pass time back to simulator
      //
      trap_write_block (_cpu, (char*)(&arc_tv), a0, sizeof(struct __arc_timeval));
#else    
      // On 32-bit simulation host
      //
      struct timezone host_tz;

      // Retrieve timezone
      //
			trap_read_block (_cpu, (char*)(&host_tz), a1, sizeof(struct timezone));
      // Actually get timeofday
      //
			res = gettimeofday (&host_tv, &host_tz);
      // Pass time back to simulator
      //
			trap_write_block (_cpu, (char*)(&host_tv), a0, sizeof(struct timeval));
#endif
			if (res == (uint32)-1)
				res = -errno;
      break;
    }
      
    case __ARC_LINUX_NR_time:
		{
			uint64 insns = clock_ticks();
			uint32 secs = (uint32)(insns/CLOCK_FREQUENCY);
			res = secs;
		}
      break;
      
    case __ARC_LINUX_NR_exit:
		{
			halt_cpu();
		}
      break;
      
    case __ARC_LINUX_NR_sigprocmask:
    {
      
#if (defined(__APPLE__) && defined(__MACH__))
      res = sigprocmask(SYS_sigprocmask, (const sigset_t *)a1, (sigset_t *)a2);
#else
      res = sigprocmask(a0, (const sigset_t *)a1, (sigset_t *)a2);
#endif
      break;
    }
    case __ARC_LINUX_NR_sigaction:
#if (defined(__APPLE__) && defined(__MACH__))
      res = sigaction(SYS_sigaction, (const struct sigaction *)a1, (struct sigaction *) a2);
#else
      res = sigaction(a0, (const struct sigaction *)a1, (struct sigaction *) a2);
#endif      
      break;
      
    case __ARC_LINUX_NR_rt_sigaction:
    {
#if (defined(__APPLE__) && defined(__MACH__))
      res = syscall(SYS_sigaction, a0, a1, a2, a3);
#else
      res = syscall(__ARC_LINUX_NR_rt_sigaction, a0, a1, a2, a3);
#endif
      break;
    }
      
    case __ARC_LINUX_NR_getcwd:
      // N.B. The linux system call (man 2 getcwd) has different semantics from
      //      the libc getcwd function (man 3 getcwd), so we need to translate.
    {
      char fname[a1];
      char *retval = getcwd(fname, a1);
      trap_write_block (_cpu, fname, a0, a1);
      if (retval == NULL) {
        res = -errno;
      } else {
        res = (uint32) strlen(fname) + 1;
      }
    }
      break;
      
    case __ARC_LINUX_NR_getpid:
    {
      res = getpid();
      break;
    }
      
    case __ARC_LINUX_NR_getuid32:
    {
      res = getuid();
      break;
    }
      
    case __ARC_LINUX_NR_getgid32:
    {
      res = getgid();
      break;
    }
      
    case __ARC_LINUX_NR_geteuid32:
    {
      
      res = geteuid();
      break;
    }
      
    case __ARC_LINUX_NR_getegid32:
    {
      
      res = getegid();
      break;
    }
      
    case __ARC_LINUX_NR_kill:
    {
      
      if ( getpid() == (pid_t)a0 ) {
        res =  kill( (pid_t)a0, (int)a1);
      } else {
        res = (uint32)-1;
      }     
      break;
    }
      
    case __ARC_LINUX_NR_readlink:
    { 
      
      /* Arguments: char *path [a0], char *buf [a1], size_t bufsiz [a2] */
      char path[256];
      char buf[a2];
			trap_read_block (_cpu, path, a0, 256);
      res = (uint32)readlink(path, buf, a2);
      if (res == (uint32)-1) {
        res = -errno;
      } else {
        trap_write_block (_cpu, buf, a1, res);
      }
    }
      break;
      
    default:
		{
      LOG(LOG_ERROR) << "[CPU" << _cpu.core_id << "] *** unimplemented syscall ("
                     << call << ") ***";
			res = (uint32)-1; /* Operation not permitted */
		}
	}
}

