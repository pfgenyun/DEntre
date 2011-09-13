/************************************************************
 * Copyright (c) 2010-present Peng Fei.  All rights reserved.
 ************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistribution and use in source and binary forms must authorized by
 * Peng Fei.
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>		/* ENOENT */
#include <sys/sysinfo.h>	/* for get_nprocs_num  */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <string.h>

#include "../globals.h"
#include "syscall.h"
#include "../utils.h"
#include "../mips/proc.h"
#include "os_private.h"
#include "../vmareas.h"
#include "../heap.h"

#include <stddef.h>	/* for offsetof */

/* must be after N64 is defined */
#ifdef N64
# define SYSNUM_STAT SYS_stat
# define SYSNUM_FSTAT SYS_fstat
#else
# define SYSNUM_STAT SYS_stat64
# define SYSNUM_FSTAT SYS_fstat64
#endif


#ifndef HAVE_TLS
/* We use a table lookup to find a thread's dcontext */
/* Our only current no-TLS target, VMKernel (VMX86_SERVER), doesn't have apps with
 * tons of threads anyway
 */
#define MAX_THREADS 512
typedef struct _tls_slot_t {
    thread_id_t tid;
    dcontext_t *dcontext;
} tls_slot_t;
/* Stored in heap for self-prot */
static tls_slot_t *tls_table;
/* not static so deadlock_avoidance_unlock() can look for it */
DECLARE_CXTSWPROT_VAR(mutex_t tls_lock, INIT_LOCK_FREE(tls_lock));
#endif


/* does the kernel provide tids that must be used to distinguish threads in a group? */
static bool kernel_thread_groups;

static bool kernel_64bit;

pid_t pid_cached;


/* Track all memory regions seen by DR. We track these ourselves to prevent
 * repeated reads of /proc/self/maps (case 3771). An allmem_info_t struct is
 * stored in the custom field.
 * all_memory_areas is updated along with dynamo_areas, due to cyclic
 * dependencies.
 */
static vm_area_vector_t *all_memory_areas;

static int
get_library_bounds(const char *name, app_pc *start/*IN/OUT*/, app_pc *end/*OUT*/,
                   char *fullpath/*OPTIONAL OUT*/, size_t path_size);

static void
get_uname(void)
{
    /* assumption: only called at init, so we don't need any synch
     * or .data unprot
     */
	static struct utsname uinfo;
	
	DEBUG_DECLARE(int res =)
		dentre_syscall(SYS_uname, 1, (ptr_uint_t)&uinfo);

    LOG(GLOBAL, LOG_TOP, 1, "uname:\n\tsysname: %s\n", uinfo.sysname);
    LOG(GLOBAL, LOG_TOP, 1, "\tnodename: %s\n", uinfo.nodename);
    LOG(GLOBAL, LOG_TOP, 1, "\trelease: %s\n", uinfo.release);
    LOG(GLOBAL, LOG_TOP, 1, "\tversion: %s\n", uinfo.version);
    LOG(GLOBAL, LOG_TOP, 1, "\tmachine: %s\n", uinfo.machine);
    if (strncmp(uinfo.machine, "mips64", sizeof("mips64")) == 0)
        kernel_64bit = true;
}


/* vmvector callbacks */
static void 
allmem_info_free(void *data)
{
	/* need to be filled up */
}

static void *
allmem_info_dup(void * data)
{
	/* need to be filled up */
}

static bool 
allmem_should_merge(bool adjacent, void *data1, void *data2)
{
	/* need to be filled up  */
}

static void *
allmem_info_merge(void *dst_data, void *src_data)
{
	/* need to be filled up */
}


/* os-specific initializations */
void os_init(void)
{
	get_uname();

    /* determine whether gettid is provided and needed for threads,
     * or whether getpid suffices.  even 2.4 kernels have gettid
     * (maps to getpid), don't have an old enough target to test this.
     */
	kernel_thread_groups = (dentre_syscall(SYS_gettid, 0) >= 0);
    LOG(GLOBAL, LOG_TOP|LOG_STATS, 1, "thread id is from %s\n",
        kernel_thread_groups ? "gettid" : "getpid");
    ASSERT_CURIOSITY(kernel_thread_groups);

	pid_cached = get_process_id();
	signal_init();

#ifdef PROFILE_RDTSC
    if (dentre_options.profile_times) {
        ASSERT_NOT_TESTED();
        kilo_hertz = get_timer_frequency();
        LOG(GLOBAL, LOG_TOP|LOG_STATS, 1, "CPU MHz is %d\n", kilo_hertz/1000);
    }
#endif /* PROFILE_RDTSC */

	/* Need to be after heap_init */
	VMVECTOR_ALLOC_VECTOR(all_memory_areas, GLOBAL_DCONTEXT, VECTOR_SHARED, all_memory_areas);
	vmvector_set_callbacks(all_memory_areas, allmem_info_free, allmem_info_dup,
						   allmem_should_merge, allmem_info_merge);
}


process_id_t
get_process_id()
{
	dentre_syscall(SYS_getpid, 0);
}

static char *
get_application_name_helper(bool ignore_cache)
{
	static char name_buf[MAXINUM_PATH];
	
	if(!name_buf[0] || ignore_cache)
	{
		extern void getnamefrompid(int pid, char *name, uint maxlen);
		getmamefrompid(get_process_id(), name_buf, BUFFER_SIZE_ELEMENTS(name_buf));
	}

	return name_buf;
}

char *
get_application_name(void)
{
	return get_application_name_helper(false);
}

DENTRE_EXPORT const char *
get_application_short_name()
{
	return get_application_name();
}


int
open_syscall(const char *file, int flags, int mode)
{
    ASSERT(file != NULL);
    return dentre_syscall(SYS_open, 3, file, flags, mode);
}

int
close_syscall(int fd)
{
    return dentre_syscall(SYS_close, 1, fd);
}

int
dup_syscall(int fd)
{
    return dentre_syscall(SYS_dup, 1, fd);
}

ssize_t
read_syscall(int fd, void *buf, size_t nbytes)
{
    return dentre_syscall(SYS_read, 3, fd, buf, nbytes);
}

ssize_t
write_syscall(int fd, const void *buf, size_t nbytes)
{
    return dentre_syscall(SYS_write, 3, fd, buf, nbytes);
}


#if defined(CLIENT_INTERFACE) || defined(HOT_PATCHING_INTERFACE)
shlib_handle_t 
load_shared_library(char *name)
{
    return dlopen(name, RTLD_LAZY);
}
#endif

#if defined(CLIENT_INTERFACE)
shlib_routine_ptr_t
lookup_library_routine(shlib_handle_t lib, char *name)
{
    return dlsym(lib, name);
}

void
unload_shared_library(shlib_handle_t lib)
{
    if (!DYNAMO_OPTION(avoid_dlclose))
        dlclose(lib);
}

void
shared_library_error(char *buf, int maxlen)
{
    char *err = dlerror();
    strncpy(buf, err, maxlen-1);
    buf[maxlen-1] = '\0'; /* strncpy won't put on trailing null if maxes out */
}

/* addr is any pointer known to lie within the library */
bool
shared_library_bounds(IN shlib_handle_t lib, IN byte *addr,
                      OUT byte **start, OUT byte **end)
{
    /* PR 366195: dlopen() handle truly is opaque, so we have to use addr */
    ASSERT(start != NULL && end != NULL);
    *start = addr;
    return (get_library_bounds(NULL, start, end, NULL, 0) > 0);
}
#endif /* defined(CLIENT_INTERFACE) */


/* FIXME - not available in 2.0 or earlier kernels, not really an issue since no one
 * should be running anything that old. */
int
llseek_syscall(int fd, int64 offset, int origin, int64 *result)
{
#ifdef N64
    *result = dentre_syscall(SYS_lseek, 3, fd, offset, origin);
    return ((*result > 0) ? 0 : (int)*result);
#else
    return dentre_syscall(SYS__llseek, 5, fd, (uint)((offset >> 32) & 0xFFFFFFFF),
                             (uint)(offset & 0xFFFFFFFF), result, origin); 
#endif
}


/* not easily accessible in header files */
#ifdef N64
/* not needed */
# define O_LARGEFILE    0100000
#else
# define O_LARGEFILE    0
#endif


/* we assume that opening for writing wants to create file */
file_t
os_open(const char *fname, int os_open_flags)
{
    int res;
    int flags = 0;
    if (TEST(OS_OPEN_ALLOW_LARGE, os_open_flags))
        flags |= O_LARGEFILE;
    if (!TEST(OS_OPEN_WRITE, os_open_flags)) 
        res = open_syscall(fname, flags|O_RDONLY, 0);
    else {    
        res = open_syscall(fname, flags|O_RDWR|O_CREAT|
                           (TEST(OS_OPEN_APPEND, os_open_flags) ? 
                            O_APPEND : 0)|
                           (TEST(OS_OPEN_REQUIRE_NEW, os_open_flags) ? 
                            O_EXCL : 0), 
                           S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    }
    if (res < 0)
        return INVALID_FILE;
    return res;
}

file_t
os_open_directory(const char *fname, int os_open_flags)
{
    /* no special handling */
    return os_open(fname, os_open_flags);
}

void
os_close(file_t f)
{
    close_syscall(f);
}

ssize_t
os_write(file_t f, const void *buf, size_t count)
{
    return write_syscall(f, buf, count);
}

ssize_t 
os_read(file_t f, void *buf, size_t count)
{
    return read_syscall(f, buf, count);
}

void
os_flush(file_t f)
{
    /* we're not using FILE*, so there is no buffering */
}

/* seek the current file position to offset bytes from origin, return true if successful */
bool
os_seek(file_t f, int64 offset, int origin)
{
    int64 result;
    int ret = 0;

    ret = llseek_syscall(f, offset, origin, &result);

    return (ret == 0);
}

/* return the current file position, -1 on failure */
int64
os_tell(file_t f)
{
    int64 result = -1;
    int ret = 0;

    ret = llseek_syscall(f, 0, SEEK_CUR, &result);

    if (ret != 0)
        return -1;

    return result;
}

bool
os_delete_file(const char *name)
{
    return (dentre_syscall(SYS_unlink, 1, name) == 0);
}

bool
os_rename_file(const char *orig_name, const char *new_name, bool replace)
{
    ptr_int_t res;
    if (!replace) {
        /* SYS_rename replaces so we must test beforehand => could have race */
        /* _LARGEFILE64_SOURCE should make libc struct match kernel (see top of file) */
//        struct stat64 st;   /* maybe error here */
        struct stat st;
        ptr_int_t res = dentre_syscall(SYSNUM_STAT, 2, new_name, &st);
        if (res == 0)
            return false;
        else if (res != -ENOENT) {
            LOG(THREAD_GET, LOG_SYSCALLS, 2, "%s stat failed: "PIFX"\n", __func__, res);
            return false;
        }
    }
    res = dentre_syscall(SYS_rename, 2, orig_name, new_name);
    if (res != 0)
        LOG(THREAD_GET, LOG_SYSCALLS, 2, "%s \"%s\" to \"%s\" failed: "PIFX"\n",
            __func__, orig_name, new_name, res);
    return (res == 0);
}

bool
os_delete_mapped_file(const char *filename)
{
    return os_delete_file(filename);
}

/* since 1970 */
uint 
query_time_seconds()
{
	return (uint) dentre_syscall(SYS_time, 1, NULL);
}

int
get_num_processors()
{
	static uint num_cpu = 0;
	if(!num_cpu)
	{
		num_cpu = get_nprocs_conf();
		ASSERT(num_cpu);
	}

	return num_cpu;
}


static inline uint
memprot_to_osprot(uint prot)
{
	uint mmap_prot = 0;
	if(TEST(MEMPROT_EXEC, prot))
		mmap_prot |= PROT_EXEC;
	if(TEST(MEMPROT_READ, prot))
		mmap_prot |= PROT_READ;
	if(TEST(MEMPROT_WRITE, prot))
		mmap_prot |= PROT_WRITE;

	return mmap_prot;
}


static int
get_library_bounds(const char *name, app_pc *start/*IN/OUT*/, app_pc *end/*OUT*/,
                   char *fullpath/*OPTIONAL OUT*/, size_t path_size)
{
	/* need to filled up */
}



static bool
mmap_syscall_succeeded(byte *retval)
{
    ptr_int_t result = (ptr_int_t) retval;
    /* libc interprets up to -PAGE_SIZE as an error, and you never know if
     * some weird errno will be used by say vmkernel (xref PR 365331) 
     */
    bool fail = (result < 0 && result >= -PAGE_SIZE);
    ASSERT_CURIOSITY(!fail ||
                     IF_VMX86(result == -ENOENT ||)
                     IF_VMX86(result == -ENOSPC ||)
                     result == -EBADF   ||
                     result == -EACCES  ||
                     result == -EINVAL  ||
                     result == -ETXTBSY ||
                     result == -EAGAIN  ||
                     result == -ENOMEM  ||
                     result == -ENODEV  ||
                     result == -EFAULT);
    return !fail;
}

static inline byte *
mmap_syscall(byte * addr, size_t len, ulong prot, ulong flag, ulong fd, ulong pgoff)
{
	return (byte *) dentre_syscall(IF_N64_ELSE(SYS_mmap, SYS_mmap2), 6,
									addr, len, prot, flag, fd, pgoff);
}

static inline long
munmap_syscall(byte *addr, size_t len)
{
	dentre_syscall(SYS_munmap, 2, addr, len);
}


static inline long
mprotect_syscall(byte *p, size_t size, uint prot)
{
	return dentre_syscall(SYS_mprotect, 3, p, size, prot);
}

void 
os_heap_free(void *p, size_t size, heap_error_code_t *error_code)
{
	long rc;
	ASSERT(error_code != NULL);

	if(!dentre_exited)
	{
		LOG(GLOBAL, LOG_HEAP, 4, "os_heap_free: %d bytes @ "PFX"\n", size, p);
	}

	rc = munmap_syscall(p, size);

	if(rc != 0)
	{
		*error_code = -rc;
	}
	else
	{
		*error_code = HEAP_ERROR_SUCCESS;
	}

	ASSERT(rc == 0);
}

/* reserve virtual address space without committing swap space for it, 
   and of course no physical pages since it will never be touched */
/* to be transparent, we do not use sbrk, and are
 * instead using mmap, and asserting that all os_heap requests are for
 * reasonably large pieces of memory */
void *
os_heap_reserve(void *preferred, size_t size, heap_error_code_t *error_code, bool executable)
{
	void *p;
	uint prot = PROT_NONE;

	ASSERT(size > 0 && ALIGNED(size, PAGE_SIZE));
	ASSERT(error_code != NULL);

	p = mmap_syscall(preferred, size, prot, MAP_PRIVATE|MAP_ANONYMOUS 
			IF_N64(| (DENTRE_OPTION(heap_in_lower_4GB) ? MAP_32BIT: 0)), -1, 0);

	if(!mmap_syscall_succeeded(p))
	{
		*error_code = -(heap_error_code_t)(ptr_int_t)p;
        LOG(GLOBAL, LOG_HEAP, 4,
            "os_heap_reserve %d bytes failed "PFX"\n", size, p);
        return NULL;
	}
	else if(preferred = NULL && p != preferred)
	{
		heap_error_code_t dummy;
		*error_code = HEAP_ERROR_NOT_AT_PREFERRED;
		os_heap_free(p, size, &dummy);
		ASSERT(dummy == HEAP_ERROR_SUCCESS);
        LOG(GLOBAL, LOG_HEAP, 4,
            "os_heap_reserve %d bytes at "PFX" not preferred "PFX"\n",
            size, preferred, p);
        return NULL;
	}
	else
	{
		*error_code = HEAP_ERROR_SUCCESS;
	}
    LOG(GLOBAL, LOG_HEAP, 2, "os_heap_reserve: %d bytes @ "PFX"\n", size, p);

	return p;
}


/* commit previously reserved with os_heap_reserve pages */
/* returns false when out of memory */
/* A replacement of os_heap_alloc can be constructed by using os_heap_reserve 
   and os_heap_commit on a subset of the reserved pages. */
/* caller is required to handle thread synchronization */
bool
os_heap_commit(void *p, size_t size, uint prot, heap_error_code_t *error_code)
{
	uint os_prot = memprot_to_osprot(prot);

	long res;

	ASSERT(size > 0 && ALIGNED(size, PAGE_SIZE));
	ASSERT(p);
	ASSERT(error_code != NULL);

	res = mprotect_syscall(p, size, os_prot);
	if(res != 0)
	{
		*error_code = -res;
		return false;
	}
	else
		return HEAP_ERROR_SUCCESS;

	LOG(GLOBAL, LOG_HEAP, 2, "os_heap_commit: %d bytes @ "PFX"\n", size, p);

	return true;
}


void
update_all_memory_areas(app_pc start, app_pc end_in, uint prot, int type)
{
	/* need to be filled up */
	/* vmarea managment  */
}


/* make pc's page unwritable 
 * FIXME: how get current protection?  would like to keep old read/exec flags
 */
void
make_unwritable(byte *pc, size_t size)
{
	/* need to be filled up */
}

thread_id_t
get_sys_thread_id()
{
	/* need to be filled up */
}

thread_id_t 
get_thread_id()
{
	/* need to be filled up  */
}

thread_id_t
get_tls_thread_id()
{
	/* need to be filled up */
}

/* returns the thread-private dcontext pointer for the calling thread */
dcontext_t *
get_thread_private_dcontext(void)
{
	/* need to be filled up */
}

/* sets the thread-private dcontext pointer for the calling thread */
void
set_thread_private_dcontext(dcontext_t *dcontext)
{
#ifdef HAVE_TLS
	ushort offs = TLS_DCONTEXT_OFFSET;
	ASSERT();
	WRITE_TLS_SLOT(offs, dcontext);		/* base reg is SEG register */
#else
	/* need to be filled up */
#endif
}

/* yield the current thread */
void 
thread_yield()
{
	dentre_syscall(SYS_sched_yield, 0);
}


/* We support 3 different methods of creating a segment (see os_tls_init()) */
typedef enum {
    TLS_TYPE_NONE,
    TLS_TYPE_LDT,
    TLS_TYPE_GDT,
#ifdef N64
    TLS_TYPE_ARCH_PRCTL,
#endif
} tls_type_t;


/* layout of our TLS */
typedef struct _os_local_state_t
{
    /* put state first to ensure that it is cache-line-aligned */
    /* On Linux, we always use the extended structure. */
	local_state_extended_t state;
	/* linear address of tls page */
	struct _os_local_state_t *self;
	/* store what type of TLS this is so we can clean up properly */
	tls_type_t tls_type;
    /* For pre-SYS_set_thread_area kernels (pre-2.5.32, pre-NPTL), each
     * thread needs its own ldt entry */
	int ldt_index;
	/* tid needed to ensure children are set up properly */
	thread_id_t tid;
	
#ifdef CLIENT_INTERFACE
    void *client_tls[MAX_NUM_CLIENT_TLS];
#endif
} os_local_state_t;

#define TLS_LOCAL_STATE_OFFSET (offsetof(os_local_state_t, state))

/* offset from top of page */
#define TLS_OS_LOCAL_STATE     0x00

#define TLS_SELF_OFFSET        (TLS_OS_LOCAL_STATE + offsetof(os_local_state_t, self))
#define TLS_THREAD_ID_OFFSET   (TLS_OS_LOCAL_STATE + offsetof(os_local_state_t, tid))
#define TLS_DCONTEXT_OFFSET    (TLS_OS_LOCAL_STATE + TLS_DCONTEXT_SLOT)


#define WRITE_TLS_SLOT(idx, var)	
#define READ_TLS_SLOT(idx, var)	

local_state_extended_t *
get_local_state_extended()
{
    os_local_state_t *os_tls;
    ushort offs = TLS_SELF_OFFSET;
    ASSERT(is_segment_register_initialized());
    READ_TLS_SLOT(offs, os_tls);
    return &(os_tls->state);
}


local_state_t *
get_local_state()
{
#ifdef HAVE_TLS
	return (local_state_t *)get_local_state_extended();
#else
	return NULL;
#endif
}


void
os_tls_init()
{
#ifdef HAVE_TLS
	byte *segment = heap_mmap(PAGE_SIZE);
	os_local_state_t *os_tls = (os_local_stat_t *) segment;

	memset(segment, 0, PAGE_SIZE);

	os_tls->self = os_tls;
	os_tls->tid = get_thread_id();
	os_tls->tls_type = TLS_TYPE_NONE;

	/* need to be filled up */

#else
	tls_table = (tls_slot_t *)
		global_heap_alloc(MAX_THREADS*sizeof(tls_slot_t) HEAPACCT(ACCT_OTHER));
	memset(tls_table, 0, MAX_THREADS*sizeof(tls_slot_t));
#endif
}


