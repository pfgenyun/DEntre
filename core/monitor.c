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

#include "globals.h"
#include "utils.h"
#include "monitor.h"
#include "fragment.h"


/* SPEC2000 applu has a trace head entry fragment of size 40K! */
/* streamit's fft had a 944KB bb (ridiculous unrolling) */
/* no reason to have giant traces, second half will become 2ndary trace */
/* The instrumentation easily makes trace large, 
 * so we should make the buffer size bigger, if a client is used.*/
#ifdef CLIENT_INTERFACE
# define MAX_TRACE_BUFFER_SIZE  MAX_FRAGMENT_SIZE
#else
# define MAX_TRACE_BUFFER_SIZE  (16*1024) /* in bytes */
#endif

/* Initialization */
/* thread-shared init does nothing, thread-private init does it all */
void
monitor_init()
{
	/* to reduce memory, we use ushorts for some offsets in fragment bodies,
	 * so we have to stop a trace at that size
	 * this does not include exit stubs
	 */
	ASSERT(MAX_TRACE_BUFFER_SIZE <= MAX_FRAGMENT_SIZE);
}
