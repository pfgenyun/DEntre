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


/* thread-shared initialization that should be repeated after a reset */
static void
fcache_reset_init(void)
{
	/* need to be filled up */
}

/* initialization -- needs no locks */
void
fcache_init()
{
	ASSERT(offsetof(fragment_t, flags) == offsetof(empty_slot_t, flags));
	/* what's this struct means ?*/
	DODEBUG(
		{
			/* ensure flag in ushort is at same spot as in uint */
		free_list_header_t free;
		free.flags = FRAG_FAKE | FRAG_FCACHE_FREE_LIST;
		ASSERT(TEST(FRAG_FCACHE_FREE_LIST, ((fragment_t *)(&free))->flags));
		/* ensure treating fragment_t* as next will work */
		ASSERT(offsetof(free_list_header_t, next) == offsetof(live_header_t, f));
		});

	ASSERT(FREE_LIST_SIZE[0] == 0);

	VMVECTOR_ALLOC_VECTOR(fcache_unit_areas, GLOBAL_DCONTEXT,
						  VECTOR_SHARED | VECTOR_NEVER_MERGE,
						  fcache_unit_areas);

	allunits = HEAP_TYPE_ALLOC(GLOBAL_DCONTEXT, fcache_list_t, ACCT_OTHER, PROTECTED);
	allunits->units = NULL;
	allunits->dead = NULL;
	allunits->num_dead = 0;
	allunits->units_to_flush = NULL;
	allunits->units_to_free = NULL;
	allunits->units_to_free_tail = NULL;

	fcache_reset_init();
}

