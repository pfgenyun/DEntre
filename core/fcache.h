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

#ifndef _FCACHE_H_
#define _FCACHE_H_	1


/* control over what to reset */
enum {
    /* everything, separate since more than sum of others */
    RESET_ALL              = 0x001,
    /* NYI (case 6335): just bb caches + heap */
    RESET_BASIC_BLOCKS     = 0x002,
    /* NYI (case 6335): just trace caches + heap */
    RESET_TRACES           = 0x004,
    /* just pending deletion entries (-reset_every_nth_pending)
     * TODO OPTIMIZATION (case 7147): we could avoid suspending
     * everyone and only suspend those threads w/ low flushtimes.
     */
    RESET_PENDING_DELETION = 0x008,
};


extern mutex_t reset_pending_lock;

bool schedule_reset(uint target);

void fcache_low_on_memory();

void fcache_init(void);

void
fcache_thread_init(dcontext_t *dcontext);



#endif
