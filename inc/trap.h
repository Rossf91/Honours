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
//  linux system calls made via uclibc i/o functions.
//
// =====================================================================

#ifndef _trap_h_
#define _trap_h_

#if (defined(__APPLE__) && defined(__MACH__))
/* Darwin misses the following definitions:
 http://www.coda.cs.cmu.edu/~jaharkes/coda-doc/coda-src/pioctl_8h-source.html */
#define _IOC_NRBITS     8
#define _IOC_TYPEBITS   8
#define _IOC_SIZEBITS   14
#define _IOC_DIRBITS    2

#define _IOC_NRMASK     ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK   ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK   ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK    ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT+_IOC_SIZEBITS)
/* Direction bits. */
#define _IOC_NONE       0U
#define _IOC_WRITE      1U
#define _IOC_READ       2U
/* used to decode ioctl numbers.. */
#define _IOC_DIR(nr)            (((nr) >> _IOC_DIRSHIFT)  & _IOC_DIRMASK)
#define _IOC_TYPE(nr)           (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)             (((nr) >> _IOC_NRSHIFT)   & _IOC_NRMASK)
#define _IOC_SIZE(nr)           (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)
#endif /* (defined(__APPLE__) && defined(__MACH__)) */

#define CLOCK_FREQUENCY 1000000

/* We want all of the following low-level kernel structures to be 32-bit aligned, as
 * expected on the simulated ARC system. */

#pragma pack(push)
#pragma pack(4)

/* This is struct kernel_stat from uClibc-0.9.30/libc/sysdeps/linux/arc/bits/kernel_stat.h,
 * with element names changed to arcsim_... because on Darwin some st_* are macros.. WTF!
 */

struct __arc_kernel_stat {
  uint16 arcsim_dev;
  uint16 __pad1;
  uint32 arcsim_ino;
  uint16 arcsim_mode;
  uint16 arcsim_nlink;
  uint16 arcsim_uid;
  uint16 arcsim_gid;
  uint16 arcsim_rdev;
  uint16 __pad2;
  uint32 arcsim_size;
  uint32 arcsim_blksize;
  uint32 arcsim_blocks;
  uint32 arcsim_atime;
  uint32 __unused1;
  uint32 arcsim_mtime;
  uint32 __unused2;
  uint32 arcsim_ctime;
  uint32 __unused3;
  uint32 __unused4;
  uint32 __unused5;
};

/* This is struct kernel_stat64 from uClibc-0.9.30/libc/sysdeps/linux/arc/bits/kernel_stat.h */
struct __arc_kernel_stat64 {
  uint16 arcsim_dev;
  unsigned char  __pad0[10];
  uint32  __arcsim_ino;
  uint32  arcsim_mode;
  uint32  arcsim_nlink;
  uint32  arcsim_uid;
  uint32  arcsim_gid;
  uint16  arcsim_rdev;
  unsigned char  __pad3[10];
  uint64  arcsim_size;
  uint32  arcsim_blksize;
  uint32  arcsim_blocks;
  uint32  __pad4; 
  uint32  arcsim_atime;
  uint32  __pad5;
  uint32  arcsim_mtime;
  uint32  __pad6;
  uint32  arcsim_ctime;
  uint32  __pad7;
  uint64  arcsim_ino;
};

/* This is struct iovec from uClibc, adapted to arc types */

struct __arc_iovec
{
  uint32 iov_base;  // void *
  uint32 iov_len;   // size_t
};

/* This is struct tms from uClibc, adapted to arc types */

struct __arc_tms
{
  uint32 tms_utime;     /* clock_t */
  uint32 tms_stime;     /* clock_t */
  uint32 tms_cutime;    /* clock_t */
  uint32 tms_cstime;    /* clock_t */
};

struct __arc_timeval
{
  uint32 tv_sec;  /* Seconds      - time_t      */
  uint32 tv_usec; /* Microseconds - suseconds_t */
};


#pragma pack(pop) /* restore previous alignment */

#endif
