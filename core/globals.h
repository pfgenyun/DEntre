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

#ifndef _GLOBALS_H_
#define _GLOBALS_H_	1

#include "lib/globals_shared.h"
#include "linux/os_exports.h"
#include "utils.h"
#include "lib/de_stats.h"

#define INVALID_THREAD_ID  0

#define SUCCESS	(1)
#define FAILURE	(0)

#ifdef USE_VISIBILITY_ATTRIBUTES
#	define DENTRE_EXPORT	__attribute__ ((visibility ("protected")))
#else
#define DENTRE_EXPORT	
#endif


extern bool dentre_exited;

/* global instance of statistics struct */
extern de_statistics_t *stats;

struct _dcontext_t;
typedef struct _dcontext_t dcontext_t;

struct _dcontext_t
{
	/* need to be filled up */
};


#define GLOBAL_DCONTEXT	((dcontext_t *)PTR_UINT_MINUS_1)

#endif
