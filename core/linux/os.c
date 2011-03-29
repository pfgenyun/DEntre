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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../globals.h"
#include "syscall.h"

static pid_t
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



