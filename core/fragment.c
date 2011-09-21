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
#include "fragment.h"
#include "heap.h"


/* Global count of flushes, used as a timestamp for shared deletion.
 * Reads may be done w/o a lock, but writes can only be done
 * via increment_global_flushtime() while holding shared_cache_flush_lock.
 */
DECLARE_FREQPROT_VAR(uint flushtime_global, 0);


/* allows independent sequences of flushes and delayed deletions,
 * though with -syscalls_synch_flush additions we now hold this
 * throughout a flush.
 */
DECLARE_CXTSWPROT_VAR(mutex_t shared_cache_flush_lock,
                      INIT_LOCK_FREE(shared_cache_flush_lock));


/* These global tables are kept on the heap for selfprot (case 7957) */

/* synchronization to these tables is accomplished via read-write locks,
 * where the writers are removal and resizing -- addition is atomic to
 * readers.
 * for now none of these are read from ibl routines so we only have to
 * synch with other DR routines
 */

/* need to be filled up */
static void *shared_bb;
//static fragment_table_t *shared_bb;
static void *shared_trace;
//static fragment_table_t *shared_trace;

/* if we have either shared bbs or shared traces we need this shared: */
static void *shared_future;
//static fragment_table_t *shared_future;

/* Thread-shared tables are allocated in a shared per_thread_t.
 * The structure is also used if we're dumping shared traces.
 * Kept on the heap for selfprot (case 7957)
 */
static per_thread_t *shared_pt;



#define USE_SHARED_PT() (SHARED_IBT_TABLES_ENABLED() || \
		(TRACEDUMP_ENABLED() && DENTRE_OPTION(shared_traces)))

/* thread-shared initialization that should be repeated after a reset */
void
fragment_reset_init()
{
    if (RUNNING_WITHOUT_CODE_CACHE())
        return;

	mutex_lock(&shared_cache_flush_lock);
    /* ASSUMPTION: a reset frees all deletions that use flushtimes, so we can
     * reset the global flushtime here
     */
    flushtime_global = 0;
    mutex_unlock(&shared_cache_flush_lock);

	/* need to be filled up */
}


/* thread-shared initialization */
void 
fragment_init()
{
    if (RUNNING_WITHOUT_CODE_CACHE())
        return;

    /* make sure fields are at same place */
    ASSERT(offsetof(fragment_t, flags) == offsetof(future_fragment_t, flags));
    ASSERT(offsetof(fragment_t, tag) == offsetof(future_fragment_t, tag));

    /* ensure we can read this w/o a lock: no cache line crossing, please */
    ASSERT(ALIGNED(&flushtime_global, 4));


//    if (SHARED_FRAGMENTS_ENABLED()) {
//        /* tables are persistent across resets, only on heap for selfprot (case 7957) */
//        if (DENTRE_OPTION(shared_bbs)) {
//            shared_bb = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fragment_table_t,
//                                        ACCT_FRAG_TABLE, PROTECTED);
//        }
//        if (DENTRE_OPTION(shared_traces)) {
//            shared_trace = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fragment_table_t,
//                                           ACCT_FRAG_TABLE, PROTECTED);
//        }
//        shared_future = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fragment_table_t,
//                                        ACCT_FRAG_TABLE, PROTECTED);
//    }
//
//    if (USE_SHARED_PT())
//        shared_pt = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, per_thread_t, ACCT_OTHER, PROTECTED);
//
//    if (SHARED_IBT_TABLES_ENABLED()) {
//        dead_lists =
//            HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT,  dead_table_lists_t, ACCT_OTHER, PROTECTED);
//        memset(dead_lists, 0, sizeof(*dead_lists));
//    }

	fragment_reset_init();

#if defined(INTERNAL) || defined(CLIENT_INTERFACE)
	/* need to be filled up */
#endif
}


void 
fragment_thread_reset_init(dcontext_t *dcontext)
{
	/* need to be filled up */
}

void 
fragment_thread_init(dcontext_t *dcontext)
{
    /* we allocate per_thread_t in the global heap solely for self-protection,
     * even when turned off, since even with a lot of threads this isn't a lot of
     * pressure on the global heap
     */
	per_thread_t *pt;

    /* don't initialize un-needed data for hotp_only & thin_client.
     * FIXME: could set htable initial sizes to 0 for all configurations, instead.
     * per_thread_t is pretty big, so we avoid it, though it costs us checks for
     * hotp_only in the islinking-related routines.
     */
    if (RUNNING_WITHOUT_CODE_CACHE())
        return;

	pt = (per_thread_t *)global_heap_alloc(sizeof(per_thread_t) HEAPACCT(ACCT_OTHER));
	dcontext->fragment_field = (void *) pt;

	framgment_reset_init(dcontext);
}
