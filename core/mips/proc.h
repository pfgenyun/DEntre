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
 * proc.h - processor implementation specific interfaces
 */


#ifndef _PROC_H_
#define _PROC_H_	1

#include "../globals.h"

#define PAGE_SIZE	(4 * 1024)

extern size_t cache_line_size;

#define CACHE_LINE_SIZE	cache_line_size

void proc_init(void);

size_t 
proc_get_cache_line_size();

ptr_uint_t
proc_bump_to_end_of_cache_line(ptr_uint_t sz);

#endif
