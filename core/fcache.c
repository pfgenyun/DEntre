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

DECLARE_CXTSWPROT_VAR(mutex_t reset_pending_lock, INIT_LOCK_FREE(reset_pending_lock));

/* indicates a call to fcache_reset_all_caches_proactively() is pending in dispatch */
DECLARE_FREQPROT_VAR(uint reset_pending, 0);


void 
fcache_low_on_memory()
{
	/* need to be filled up */
}


/* returns true if specified target wasn't already scheduled for reset */
bool
schedule_reset(uint target)
{
	bool added_target;
	ASSERT(target != 0);

	mutex_lock(&reset_pending_lock);

	added_target = !TESTALL(target, reset_pending);
	reset_pending |= target;

	mutex_unlock(&reset_pending_lock);

	return added_target;
}

