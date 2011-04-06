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
//#include <unistd.h>
#include <sys/sysinfo.h>	/* for get_nprocs_num  */

#include "../globals.h"
#include "syscall.h"
#include "../utils.h"


/* must be after X64 is defined */
#ifdef N64
# define SYSNUM_STAT SYS_stat
# define SYSNUM_FSTAT SYS_fstat
#else
# define SYSNUM_STAT SYS_stat64
# define SYSNUM_FSTAT SYS_fstat64
#endif

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
