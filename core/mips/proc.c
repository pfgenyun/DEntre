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


/*
 * proc.c - processor-specific routines
 */


#include "../globals.h"

size_t cache_line_size = 32;

void 
proc_init(void)
{
	/* need to be filled up */
}


size_t
proc_get_cache_line_size()
{
	return cache_line_size;
}


/* Given an address or number of bytes sz, return a number >= sz that is divisible by the cache line size. */
ptr_uint_t
proc_bump_to_end_of_cache_line(ptr_uint_t sz)
{
	/* need to be filled up */
}
