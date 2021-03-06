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
 * thread.c - thread synchronization
 */

#include "globals.h"
#include "heap.h"

/* Thread-local data
 */
typedef struct _thread_synch_data_t
{
	/* need to be filled up */
}thread_synch_data_t;

void
synch_init(void)
{
	/* nothing */
}

void
synch_thread_init(dcontext_t *dcontext)
{
	thread_synch_data_t *tsd = (thread_synch_data_t *)
		heap_alloc(dcontext, sizeof(thread_synch_data_t) HEAPACCT(ACCT_OTHER));
	dcontext->synch_field = (void *) tsd;

	/* need to be filled up */
}
