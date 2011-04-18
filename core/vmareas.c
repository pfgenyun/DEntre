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
#include "vmareas.h"
#include "heap.h"


enum {
    /* VM_ flags to distinguish region types 
     * We also use some FRAG_ flags (but in a separate field so no value space overlap)
     * Adjacent regions w/ different flags are never merged.
     */
    VM_WRITABLE     = 0x0001,    /* app memory writable? */
    /* UNMOD_IMAGE means the region was mmapped in and has been read-only since then
     * this excludes even loader modifications (IAT update, relocate, etc.) on win32!
     */
    VM_UNMOD_IMAGE  = 0x0002,
    VM_DELETE_ME    = 0x0004,    /* on delete queue -- for thread-local only */
     /* NOTE : if a new area is added that overlaps an existing area with a 
      * different VM_WAS_FUTURE flag, the areas will be merged with the flag 
      * taken from the new area, see FIXME in add_vm_area */
    VM_WAS_FUTURE   = 0x0008,    /* moved from future list to exec list */
    VM_DR_HEAP      = 0x0010,    /* DR heap area */
    VM_ONCE_ONLY    = 0x0020,    /* on future list but should be removed on 
                                  * first exec */
    /* FIXME case 7877, 3744: need to properly merge pageprot regions with
     * existing selfmod regions before we can truly separate this.  For now we
     * continue to treat selfmod as pageprot.
     */
    VM_MADE_READONLY = VM_WRITABLE/* FIXME: should be 0x0040 -- see above */,
                                 /* DR has marked this region read 
                                  * only for consistency, should only be used
                                  * in conjunction with VM_WRITABLE */
    VM_DELAY_READONLY = 0x0080,  /* dr has not yet marked this region read 
                                  * only for consistency, should only be used
                                  * in conjunction with VM_WRITABLE */
#ifdef PROGRAM_SHEPHERDING
    /* re-verify this region for code origins policies every time it is
     * encountered.  only used with selfmod regions that are only allowed if
     * they match patterns, to prevent other threads from writing non-pattern
     * code though and executing after the region has been approved.
     * xref case 4020.  can remove once we split code origins list from
     * cache consistency list (case 3744).
     */
    VM_PATTERN_REVERIFY = 0x0100,  
#endif

    VM_DRIVER_ADDRESS   = 0x0200,
    /* a driver hooker area, needed for case 9022.  Note we can
     * normally read properties only of user mode addresses, so we
     * have to probe addresses in this area.  Also note that we're
     * still executing all of this code in user mode e.g. there is no
     * mode switch, no conforming segments, etc.
     */

    /* Does this region contain a persisted cache?
     * Must also be FRAG_COARSE_GRAIN of course.
     * This is a shortcut to reading custom.client->persisted.
     * This is not guaranteed to be set on shared_data: only on executable_areas.
     */
    VM_PERSISTED_CACHE     = 0x0400,

    /* Case 10584: avoid flush synch when no code has been executed */
    VM_EXECUTED_FROM       = 0x0800,

    /* A workaround for lock rank issues: we delay adding loaded persisted
     * units to shared_data until first asked about.
     * This flags is NOT propagated on vmarea splits.
     */
    VM_ADD_TO_SHARED_DATA  = 0x1000,
};


static vm_area_vector_t *dentre_areas;


/* used to determine when we need to do another heap walk to keep
 * dynamo vm areas up to date (can't do it incrementally b/c of
 * circular dependencies).
 * protected for both read and write by dynamo_areas->lock
 */
/* Case 3045: areas inside the vmheap reservation are not added to the list,
 * so the vector is considered uptodate until we run out of reservation
 */
DECLARE_FREQPROT_VAR(static bool dentre_areas_uptodate, true);


void 
dentre_vm_areas_lock()
{
	/* need to be filled up */
}

void 
dentre_vm_areas_unlock()
{
	/* need to be filled up */
}



/* Assumes caller holds v->lock, if necessary.
 * Does not return the area added since it may be merged or split depending
 * on existing areas->
 * If a last_area points into this vector, the caller must make sure to
 *   clear or update the last_area pointer.
 *   FIXME: make it easier to keep them in synch -- too easy to add_vm_area
 *   somewhere to a thread vector and forget to clear last_area.
 * Adds a new area to v, merging it with adjacent areas of the same type.
 * A new area is only allowed to overlap an old area of a different type if it
 *   meets certain criteria (see asserts below).  For VM_WAS_FUTURE and 
 *   VM_ONCE_ONLY we may clear the flag from an existing region if the new 
 *   region doesn't have the flag and overlaps the existing region.  Otherwise 
 *   the new area is split such that the overlapping portion remains part of
 *   the old area.  This tries to keep entire new area from becoming selfmod
 *   for instance. FIXME : for VM_WAS_FUTURE and VM_ONCE_ONLY may want to split
 *   region if only paritally overlapping
 * 
 * FIXME: change add_vm_area to return NULL when merged, and otherwise
 * return the new complete area, so callers don't have to do a separate lookup
 * to access the added area.
 */
static void
add_vm_area(vm_area_vector_t *v, app_pc start, app_pc end, 
			uint vm_flags, uint frag_flags, void * data _IF_DEBUG(char *comment))
{
	/* need to be filled up */
}

/* returns true if the passed in area overlaps any known executable areas
 * Assumes caller holds v->lock, if necessary
 */
static bool 
vm_area_overlap(vm_area_vector_t *v, app_pc start, app_pc end)
{
	/* need to be filled up */
}


/* Due to circular dependencies bet vmareas and global heap, we cannot
 * incrementally keep dynamo_areas up to date.
 * Instead, we wait until people ask about it, when we do a complete
 * walk through the heap units and add them all (yes, re-adding
 * ones we've seen).
 */
static void
update_dentre_vm_areas(bool have_writelock)
{
	/* need to be filled up */
}

/* add dentre-internal area to the dentre-internal area list
 * this should be atomic wrt the memory being allocated to avoid races
 * w/ the app executing from it -- thus caller must hold DE areas write lock!
 */
bool
add_dentre_vm_area(app_pc start, app_pc end, uint prot, bool unmod_image _IF_DEBUG(char *comment))
{
	uint vm_flags = (TEST(MEMPROT_WRITE, prot) ? VM_WRITABLE : 0) |
					(unmod_image ? VM_UNMOD_IMAGE : 0);
	
	ASSERT(!is_vmm_reserved_address(start, end - start));
    LOG(GLOBAL, LOG_VMAREAS, 2, "new dentre vm area: "PFX"-"PFX" %s\n",
        start, end, comment);
	ASSERT(dentre_areas != NULL);
	ASSERT_OWN_WRITE_LOCK(true, &dentre_areas->lock);

	if(!dentre_areas_uptodate)
		update_dentre_vm_areas(true);

	ASSERT(!vm_area_overlap(dentre_areas, start, end));

	add_vm_area(dentre_areas, start, end, vm_flags, 0/* frag_flags */, NULL _IF_DEBUG(comment));

	update_all_memory_areas(start, end, prot, 
							unmod_image ? DE_MEMTYPE_IMAGE : DE_MEMTYPE_DATA);

	return true;
}


void 
up_date_all_memory_areas(app_pc start, app_pc end_in, uint prot, int type)
{
	/* need to be filled up */
}
