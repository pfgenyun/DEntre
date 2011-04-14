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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <string.h>
#include "globals.h"
#include "utils.h"
#include "options.h"	/* dentre_options */

void 
getnamefrompid(int pid, char *name, uint maxlen)
{
    int fd,n;
    char tempstring[200+1], *lastpart;

    /*this is a shitty way of getting the process name,
    but i can't think of anything better... */

    snprintf(tempstring, 200+1, "/proc/%d/cmdline", pid);
    fd = open_syscall(tempstring, O_RDONLY, 0);
    /* buffer overflow even if only off by 1 can be devastating */
    n = read_syscall(fd, tempstring, 200);   
    tempstring[n] = '\0';
    lastpart = rindex(tempstring, '/');

    if (lastpart == NULL)
      lastpart = tempstring;
    else
      lastpart++; /*don't include last '/' in name*/ 

    strncpy(name, lastpart, maxlen-1);
    name[maxlen-1]  = '\0'; /* if max no null */

    close_syscall(fd);
}


void
acquire_recursive_lock(recursive_lock_t *lock)
{
	/* need to be filled up */
}

void
release_recursive_lock(recursive_lock_t *lock)
{
	/* need to be filled up */
}


void
read_lock(read_write_lock_t *rw)
{
	/* need to be filled up */
}

void
read_unlock(read_write_lock_t *rw)
{
	/* need to be filled up */
}


void 
write_lock(read_write_lock_t *rw)
{
	/* need to be filled up */
}

void 
write_unlock(read_write_lock_t *rw)
{
	/* need to be filled up */
}

void
mutex_lock(mutex_t *lock)
{
	/* need to be filled up */
}

void
mutex_unlock(mutex_t *lock)
{
	/* need to be filled up */
}

static uint spinlock_count = 0;
DECLARE_FREQPROT_VAR(static uint random_seed, 1234);
DEBUG_DECLARE(static uint initial_random_seed);

void 
utils_init()
{
	spinlock_count = (get_num_processors() - 1) * DENTRE_OPTION(spinlock_count_on_SMP);

	random_seed = (DENTRE_OPTION(prng_seed) == 0)?
		os_randomn_seed() : DENTRE_OPTION(prng_seed);

	DODEBUG(initial_random_seed = random_seed);

	ASSERT(sizeof(spin_mutex_t) = sizeof(mutex_t));
	ASSERT(sizeof(uint64) = 8);
	ASSERT(sizeof(uint32) = 4);
	ASSERT(sizeof(uint) = 4);
	ASSERT(sizeof(reg_t) = sizeof(* void));

	if(DENTRE_OPTION(open_tcsh_fds))
	{
		file_t foo = 0;
		while(foo <=5 )
		{
			foo = os_open("/dev/null", OS_OPEN_WRITE|OS_OPEN_APPEND);
			ASSERT(foo != INVALID_FILE);
		}
	}
}

void
bitmap_initialize_free(bitmap_t b, uint bitmap_size)
{
	memset(b, 0xff, BITMAP_INDEX(bitmap_size) * sizeof(bitmap_element_t));
}


uint
bitmap_allocate_blocks(bitmap_t b, uint bitmap_size, uint request_blocks)
{
	/* need to be filled up */
}

bool 
bitmap_check_consistency(bitmap_t b, uint bitmap_size, uint expect)
{
	/* need to be filled up */
}

size_t
get_random_offset(size_t max_offset)
{
	/* need to be filled up */
}
