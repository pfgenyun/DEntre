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


/* Global count of flushes, used as a timestamp for shared deletion.
 * Reads may be done w/o a lock, but writes can only be done
 * via increment_global_flushtime() while holding shared_cache_flush_lock.
 */
DECLARE_FREQPROT_VAR(uint flushtime_global, 0);


#define USE_SHARED_PT() (SHARED_IBT_TABLES_ENABLED() || \
		(TRACEDUMP_ENABLED() && DENTRE_OPTION(shared_traces)))

/* thread-shared initialization that should be repeated after a reset */
void
fragment_reset_init()
{
    if (RUNNING_WITHOUT_CODE_CACHE())
        return;


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


    if (SHARED_FRAGMENTS_ENABLED()) {
        /* tables are persistent across resets, only on heap for selfprot (case 7957) */
        if (DENTRE_OPTION(shared_bbs)) {
            shared_bb = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fragment_table_t,
                                        ACCT_FRAG_TABLE, PROTECTED);
        }
        if (DENTRE_OPTION(shared_traces)) {
            shared_trace = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fragment_table_t,
                                           ACCT_FRAG_TABLE, PROTECTED);
        }
        shared_future = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fragment_table_t,
                                        ACCT_FRAG_TABLE, PROTECTED);
    }

    if (USE_SHARED_PT())
        shared_pt = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, per_thread_t, ACCT_OTHER, PROTECTED);

    if (SHARED_IBT_TABLES_ENABLED()) {
        dead_lists =
            HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT,  dead_table_lists_t, ACCT_OTHER, PROTECTED);
        memset(dead_lists, 0, sizeof(*dead_lists));
    }

	fragment_reset_init();

#if define(INTERNAL) || define(CLIENT_INTERFACE)
#endif
}
