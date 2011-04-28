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

#include <stddef.h>		/* offsetof */

#include "globals.h"
#include "utils.h"
#include "fragment.h"
#include "vmareas.h"
#include "heap.h"
#include "link.h"


/**************************************************
 * We use a FIFO replacement strategy
 * Empty slots in the cache are always at the front of the list, and they
 * take the form of the empty_slot_t struct.
 * Target fragments to delete are represented in the FIFO by their fragment_t
 * struct, using the next_fcache field to chain them.
 * We use a header for each fragment so we can delete adjacent-in-cache
 * (different from indirected FIFO list) to make contiguous space available.
 * Padding for alignment is always at the end of the fragment, so it can be
 * combined with empty space eaten up when deleting a fragment but only needing
 * some of its room.  We assume everything starts out aligned, and the same alignment
 * on the end of each slot keeps it aligned.
 * Thus:
 *           ------
 *           header
 *           <up to START_PC_ALIGNMENT-1 bytes padding, stored in the alignment of start_pc>
 * start_pc: prefix
 *           body
 *           stubs
 *           <padding for alignment>
 *           ------
 *           header
 *           <up to START_PC_ALIGNMENT-1 bytes padding, stored in the alignment of start_pc>
 * start_pc: ...
 */
typedef struct _empty_slot_t
{
	cache_pc start_pc;		/* very top of location in fcache */
    /* flags MUST be at same location as fragment_t->flags
     * we use flags==FRAG_IS_EMPTY_SLOT to indicate an empty slot
     */
	uint flags;

	fragment_t *next_fcache;
	fragment_t *prev_fcache;

	uint fcache_size;		/* size rounded up to cache line boundaries;
							 * not just ushort so we can merge adjacents;
							 *not size_t since each unit assumed <4GB. */
}empty_slot_t;



/**************************************************
 * Free empty slot lists of different sizes for shared caches, where we
 * cannot easily delete victims to make room in too-small slots.
 */
static const uint FREE_LIST_SIZES[] = {
    /* Since we have variable sizes, and we do not pad to bucket
     * sizes, each bucket contains free slots that are in
     * [SIZE[bucket], SIZE[bucket+1]) where the top one is infinite.  
     * case 7318 about further extensions
     *
     * Free slots in the current unit are added only if in the middle
     * of the unit (the last ones are turned back into unclaimed space).
     *
     * Tuned for bbs: smallest are 40 (with stubs), plurality are 56, distribution trails 
     * off slowly beyond that.  Note that b/c of pad_jmps we request more than
     * the final sizes.
     * FIXME: these #s are for inlined stubs, should re-tune w/ separate stubs
     * (case 7163)
     */
    0, 44, 52, 56, 64, 72, 80, 112, 172
};
#define FREE_LIST_SIZES_NUM (BUFFER_SIZE_ELEMENTS(FREE_LIST_SIZES))


/* To support physical cache contiguity walking we store both a
 * next-free and prev-free pointer and a size at the top of the empty slot, and
 * don't waste memory with an empty_slot_t data structure.
 * We also use flags field to allow distinguishing free list slots
 * from live fragment_t's (see notes by flags field below).
 *
 * Our free list coalescing assumes that a fragment_t that follows a
 * free list entry has the FRAG_FOLLOWS_FREE_ENTRY flag set.
 *
 * FIXME: could avoid heap w/ normal empty_slot_t scheme: if don't have
 * separate stubs, empty_slot_t @ 20 bytes (no start_pc) should fit in
 * any cache slot.  Could save a few MB of heap on large apps.
 * (This is case 4937.)
 *
 * FIXME: If free lists work well we could try using for private
 * caches as well, instead of the empty_slot_t-struct-on-FIFO scheme.
 * 
 * FIXME: unit pointer may be useful to avoid fcache_lookup_unit
 */
typedef struct _free_list_header_t
{
	struct _free_list_header_t *next;

    /* We arrange these two so that the FRAG_FCACHE_FREE_LIST flag will be set
     * at the proper bit as though these were a "uint flags" at the same offset
     * in the struct as fragment_t.flags.  Since no one else examines a free list
     * as though it might be a fragment_t, we don't care about the other flags.
     * We have an ASSERT in fcache_init() to ensure the byte ordering is right.
     *
     * Since we compare a fragment_t* to this inlined struct, we're really
     * comparing a fragment_t* to the first field, the next pointer.  So when we
     * de-reference the flags we're looking at the flags of the next entry in
     * the free list.  Thus to identify a free list entry we must check for
     * either NULL or for the FRAG_FCACHE_FREE_LIST flag.
     */
	ushort flags;
	ushort size;
	struct _free_list_header_t *prev;
}free_list_header;



/* To locate the fcache_unit_t corresponding to a fragment or empty slot
 * we use an interval data structure rather than waste space with a
 * backpointer in each fragment.
 * Non-static so that synch routines in os.c can check its lock before calling
 * is_pc_recreatable which calls in_fcache.
 * Kept on the heap for selfprot (case 7957).
 */
vm_area_vector_t *fcache_unit_areas;

typedef struct _fcache_unit_t
{
	cache_pc start_pc;
	cache_pc end_pc;
	cache_pc cur_pc;
	cache_pc reserved_end_pc;

	size_t size;
	bool full;

	struct _fcache *cache;
#ifdef SIDELINE
	dcontext_t *dcontext;
#endif

	bool writable;
	bool pending_free;
#ifdef DEBUG
	bool pending_flush;
#endif

	uint flushtime;

	struct _fcache_unit_t *next_global;
	struct _fcache_unit_t *prev_global;
	struct _fcache_unit_t *next_local;
}fcache_unit_t;

/*
 *  global, unique thread-shared structure: 
 */
typedef struct _fcache_list_t
{
	/* These lists are protected by allunits_lock. */
	fcache_unit_t *units;		/* list of all allocated fcache units */
	fcache_unit_t *dead;		/* list of deleted units ready for re-allocation */
    /* FIXME: num_dead duplicates stats->fcache_num_free, but we want num_dead
     * for release build too, so it's separate...can we do better?
     */
	uint num_dead;

    /* Global lists of cache units to flush and to free, chained by next_local
     * and kept on the live units list.  Protected by unit_flush_lock, NOT by
     * allunits_lock.
     * We keep these list pointers on the heap for selfprot (case 8074).
     */
    /* units to be flushed once at a safe spot */
	fcache_unit_t *units_to_flush;

    /* units to be freed once their contents are, kept sorted in
     * increasing flushtime, with a tail pointer to make appends easy
     */
	fcache_unit_t *units_to_free;
	fcache_unit_t *units_to_free_tail;
}fcache_list_t;

/* Kept on the heap for selfprot (case 7957). */
fcache_list_t *allunits;

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

