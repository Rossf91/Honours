//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2009 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file contains system call number definitions for arc-linux.
//  These are used in trap.cpp when trap emulation mode is enabled.
//
// =====================================================================


#ifndef _ARC_UNISTD_H
#define _ARC_UNISTD_H

/* system call numbers */
#define __ARC_LINUX_NR_exit		  1
#define __ARC_LINUX_NR_fork		  2
#define __ARC_LINUX_NR_read		  3
#define __ARC_LINUX_NR_write		  4
#define __ARC_LINUX_NR_open		  5
#define __ARC_LINUX_NR_close		  6
//#define __ARC_LINUX_NR_waitpid		  7
#define __ARC_LINUX_NR_creat		  8
#define __ARC_LINUX_NR_link		  9
#define __ARC_LINUX_NR_unlink		 10
#define __ARC_LINUX_NR_execve		 11
#define __ARC_LINUX_NR_chdir		 12
#define __ARC_LINUX_NR_time		 13
#define __ARC_LINUX_NR_mknod		 14
#define __ARC_LINUX_NR_chmod		 15
#define __ARC_LINUX_NR_chown		 16
#define __ARC_LINUX_NR_break		 17
#define __ARC_LINUX_NR_oldstat		 18
#define __ARC_LINUX_NR_lseek		 19
#define __ARC_LINUX_NR_getpid		 20
#define __ARC_LINUX_NR_mount		 21
#define __ARC_LINUX_NR_umount		 22
#define __ARC_LINUX_NR_setuid		 23
#define __ARC_LINUX_NR_getuid		 24
#define __ARC_LINUX_NR_stime		 25
#define __ARC_LINUX_NR_ptrace		 26
#define __ARC_LINUX_NR_alarm		 27
#define __ARC_LINUX_NR_oldfstat		 28
#define __ARC_LINUX_NR_pause		 29
#define __ARC_LINUX_NR_utime		 30
#define __ARC_LINUX_NR_stty		 31
#define __ARC_LINUX_NR_gtty		 32
#define __ARC_LINUX_NR_access		 33
#define __ARC_LINUX_NR_nice		 34
#define __ARC_LINUX_NR_ftime		 35
#define __ARC_LINUX_NR_sync		 36
#define __ARC_LINUX_NR_kill		 37
#define __ARC_LINUX_NR_rename		 38
#define __ARC_LINUX_NR_mkdir		 39
#define __ARC_LINUX_NR_rmdir		 40
#define __ARC_LINUX_NR_dup		 41
#define __ARC_LINUX_NR_pipe		 42
#define __ARC_LINUX_NR_times		 43
#define __ARC_LINUX_NR_prof		 44
#define __ARC_LINUX_NR_brk		 45
#define __ARC_LINUX_NR_setgid		 46
#define __ARC_LINUX_NR_getgid		 47
#define __ARC_LINUX_NR_signal		 48
#define __ARC_LINUX_NR_geteuid		 49
#define __ARC_LINUX_NR_getegid		 50
#define __ARC_LINUX_NR_acct		 51
#define __ARC_LINUX_NR_umount2		 52
#define __ARC_LINUX_NR_lock		 53
#define __ARC_LINUX_NR_ioctl		 54
#define __ARC_LINUX_NR_fcntl		 55
#define __ARC_LINUX_NR_mpx		 56
#define __ARC_LINUX_NR_setpgid		 57
#define __ARC_LINUX_NR_ulimit		 58
#define __ARC_LINUX_NR_oldolduname	 59
#define __ARC_LINUX_NR_umask		 60
#define __ARC_LINUX_NR_chroot		 61
#define __ARC_LINUX_NR_ustat		 62
#define __ARC_LINUX_NR_dup2		 63
#define __ARC_LINUX_NR_getppid		 64
#define __ARC_LINUX_NR_getpgrp		 65
#define __ARC_LINUX_NR_setsid		 66
#define __ARC_LINUX_NR_sigaction		 67
#define __ARC_LINUX_NR_sgetmask		 68
#define __ARC_LINUX_NR_ssetmask		 69
#define __ARC_LINUX_NR_setreuid		 70
#define __ARC_LINUX_NR_setregid		 71
#define __ARC_LINUX_NR_sigsuspend		 72
#define __ARC_LINUX_NR_sigpending		 73
#define __ARC_LINUX_NR_sethostname	 74
#define __ARC_LINUX_NR_setrlimit		 75
#define __ARC_LINUX_NR_old_getrlimit	 76
#define __ARC_LINUX_NR_getrusage		 77
#define __ARC_LINUX_NR_gettimeofday	 78
#define __ARC_LINUX_NR_settimeofday	 79
#define __ARC_LINUX_NR_getgroups		 80
#define __ARC_LINUX_NR_setgroups		 81
#define __ARC_LINUX_NR_select		 82
#define __ARC_LINUX_NR_symlink		 83
#define __ARC_LINUX_NR_oldlstat		 84
#define __ARC_LINUX_NR_readlink		 85
#define __ARC_LINUX_NR_uselib		 86
#define __ARC_LINUX_NR_swapon		 87
#define __ARC_LINUX_NR_reboot		 88
#define __ARC_LINUX_NR_readdir		 89
#define __ARC_LINUX_NR_mmap		 90
#define __ARC_LINUX_NR_munmap		 91
#define __ARC_LINUX_NR_truncate		 92
#define __ARC_LINUX_NR_ftruncate		 93
#define __ARC_LINUX_NR_fchmod		 94
#define __ARC_LINUX_NR_fchown		 95
#define __ARC_LINUX_NR_getpriority	 96
#define __ARC_LINUX_NR_setpriority	 97
#define __ARC_LINUX_NR_profil		 98
#define __ARC_LINUX_NR_statfs		 99
#define __ARC_LINUX_NR_fstatfs		100
#define __ARC_LINUX_NR_ioperm		101
#define __ARC_LINUX_NR_socketcall		102
#define __ARC_LINUX_NR_syslog		103
#define __ARC_LINUX_NR_setitimer		104
#define __ARC_LINUX_NR_getitimer		105
#define __ARC_LINUX_NR_stat		106
#define __ARC_LINUX_NR_lstat		107
#define __ARC_LINUX_NR_fstat		108
#define __ARC_LINUX_NR_olduname		109
#define __ARC_LINUX_NR_iopl		110
#define __ARC_LINUX_NR_vhangup		111
#define __ARC_LINUX_NR_idle		112
#define __ARC_LINUX_NR_vm86		113
#define __ARC_LINUX_NR_wait4		114
#define __ARC_LINUX_NR_swapoff		115
#define __ARC_LINUX_NR_sysinfo		116
#define __ARC_LINUX_NR_ipc		117
#define __ARC_LINUX_NR_fsync		118
#define __ARC_LINUX_NR_sigreturn		119
#define __ARC_LINUX_NR_clone		120
#define __ARC_LINUX_NR_setdomainname	121
#define __ARC_LINUX_NR_uname		122
#define __ARC_LINUX_NR_cacheflush		123
#define __ARC_LINUX_NR_adjtimex		124
#define __ARC_LINUX_NR_mprotect		125
#define __ARC_LINUX_NR_sigprocmask	126
#define __ARC_LINUX_NR_create_module	127
#define __ARC_LINUX_NR_init_module	128
#define __ARC_LINUX_NR_delete_module	129
#define __ARC_LINUX_NR_get_kernel_syms	130
#define __ARC_LINUX_NR_quotactl		131
#define __ARC_LINUX_NR_getpgid		132
#define __ARC_LINUX_NR_fchdir		133
#define __ARC_LINUX_NR_bdflush		134
#define __ARC_LINUX_NR_sysfs		135
#define __ARC_LINUX_NR_personality	136
#define __ARC_LINUX_NR_afs_syscall	137
#define __ARC_LINUX_NR_setfsuid		138
#define __ARC_LINUX_NR_setfsgid		139
#define __ARC_LINUX_NR__llseek		140
#define __ARC_LINUX_NR_getdents		141
#define __ARC_LINUX_NR__newselect		142
#define __ARC_LINUX_NR_flock		143
#define __ARC_LINUX_NR_msync		144
#define __ARC_LINUX_NR_readv		145
#define __ARC_LINUX_NR_writev		146
#define __ARC_LINUX_NR_getsid		147
#define __ARC_LINUX_NR_fdatasync		148
#define __ARC_LINUX_NR__sysctl		149
#define __ARC_LINUX_NR_mlock		150
#define __ARC_LINUX_NR_munlock		151
#define __ARC_LINUX_NR_mlockall		152
#define __ARC_LINUX_NR_munlockall		153
#define __ARC_LINUX_NR_sched_setparam		154
#define __ARC_LINUX_NR_sched_getparam		155
#define __ARC_LINUX_NR_sched_setscheduler		156
#define __ARC_LINUX_NR_sched_getscheduler		157
#define __ARC_LINUX_NR_sched_yield		158
#define __ARC_LINUX_NR_sched_get_priority_max	159
#define __ARC_LINUX_NR_sched_get_priority_min	160
#define __ARC_LINUX_NR_sched_rr_get_interval	161
#define __ARC_LINUX_NR_nanosleep		162
#define __ARC_LINUX_NR_mremap		163
#define __ARC_LINUX_NR_setresuid		164
#define __ARC_LINUX_NR_getresuid		165
#define __ARC_LINUX_NR_query_module	167
#define __ARC_LINUX_NR_poll		168
#define __ARC_LINUX_NR_nfsservctl		169
#define __ARC_LINUX_NR_setresgid		170
#define __ARC_LINUX_NR_getresgid		171
#define __ARC_LINUX_NR_prctl		172
#define __ARC_LINUX_NR_rt_sigreturn	173
#define __ARC_LINUX_NR_rt_sigaction	174
#define __ARC_LINUX_NR_rt_sigprocmask	175
#define __ARC_LINUX_NR_rt_sigpending	176
#define __ARC_LINUX_NR_rt_sigtimedwait	177
#define __ARC_LINUX_NR_rt_sigqueueinfo	178
#define __ARC_LINUX_NR_rt_sigsuspend	179
#define __ARC_LINUX_NR_pread		180
#define __ARC_LINUX_NR_pwrite		181
#define __ARC_LINUX_NR_lchown		182
#define __ARC_LINUX_NR_getcwd		183
#define __ARC_LINUX_NR_capget		184
#define __ARC_LINUX_NR_capset		185
#define __ARC_LINUX_NR_sigaltstack	186
#define __ARC_LINUX_NR_sendfile		187
#define __ARC_LINUX_NR_getpmsg		188
#define __ARC_LINUX_NR_putpmsg		189
#define __ARC_LINUX_NR_vfork		190
#define __ARC_LINUX_NR_getrlimit		191
#define __ARC_LINUX_NR_mmap2		192
#define __ARC_LINUX_NR_truncate64		193
#define __ARC_LINUX_NR_ftruncate64	194
#define __ARC_LINUX_NR_stat64		195
#define __ARC_LINUX_NR_lstat64		196
#define __ARC_LINUX_NR_fstat64		197
#define __ARC_LINUX_NR_chown32		198
#define __ARC_LINUX_NR_getuid32		199
#define __ARC_LINUX_NR_getgid32		200
#define __ARC_LINUX_NR_geteuid32		201
#define __ARC_LINUX_NR_getegid32		202
#define __ARC_LINUX_NR_setreuid32		203
#define __ARC_LINUX_NR_setregid32		204
#define __ARC_LINUX_NR_getgroups32	205
#define __ARC_LINUX_NR_setgroups32	206
#define __ARC_LINUX_NR_fchown32		207
#define __ARC_LINUX_NR_setresuid32	208
#define __ARC_LINUX_NR_getresuid32	209
#define __ARC_LINUX_NR_setresgid32	210
#define __ARC_LINUX_NR_getresgid32	211
#define __ARC_LINUX_NR_lchown32		212
#define __ARC_LINUX_NR_setuid32		213
#define __ARC_LINUX_NR_setgid32		214
#define __ARC_LINUX_NR_setfsuid32		215
#define __ARC_LINUX_NR_setfsgid32		216
#define __ARC_LINUX_NR_pivot_root		217
#define __ARC_LINUX_NR_getdents64		220
#define __ARC_LINUX_NR_fcntl64        221
#define __ARC_LINUX_NR_gettid		    224
#define __ARC_LINUX_NR_lookup_dcookie 225
#define __ARC_LINUX_NR_statfs64       226
#define __ARC_LINUX_NR_waitpid        227
#define __ARC_LINUX_NR_mq_open        228
#define __ARC_LINUX_NR_mq_unlink      229
#define __ARC_LINUX_NR_mq_timedreceive 230
#define __ARC_LINUX_NR_mq_notify      231
#define __ARC_LINUX_NR_mq_getsetattr  232
#define __ARC_LINUX_NR_mq_timedsend 233
#define __ARC_LINUX_NR_timer_create   234
#define __ARC_LINUX_NR_timer_settime  235
#define __ARC_LINUX_NR_timer_gettime  236
#define __ARC_LINUX_NR_timer_getoverrun 237
#define __ARC_LINUX_NR_timer_delete   238
#define __ARC_LINUX_NR_clock_settime  239
#define __ARC_LINUX_NR_clock_gettime  240
#define __ARC_LINUX_NR_clock_getres   241
#define __ARC_LINUX_NR_clock_nanosleep 242
#define __ARC_LINUX_NR_sched_setaffinity 243
#define __ARC_LINUX_NR_sched_getaffinity 244
#define __ARC_LINUX_NR_waitid         245


#endif	/* _ARC_UNISTD_H */