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

#ifndef _OS_EXPORTS_H_
#define _OS_EXPORTS_H_	1

#include <sys/types.h>
#include "../os_shared.h"


/* the VAR_IN_SECTION macro change where each var goes */
#define START_DATA_SECTION(name, wx) /* nothing */
#define END_DATA_SECTION() /* nothing */

#define VAR_IN_SECTION(name)    __attribute__ ((section (name)))

/* We do NOT want our libc routines wrapped by pthreads, so we use
 * our own syscall wrappers.
 */
int open_syscall(const char *file, int flags, int mode);
int close_syscall(int fd);
int dup_syscall(int fd);
ssize_t read_syscall(int fd, void *buf, size_t nbytes);
ssize_t write_syscall(int fd, const void *buf, size_t nbytes);

app_pc
signal_thread_inherit(dcontext_t *dcontext, void *clone_record);

#endif
