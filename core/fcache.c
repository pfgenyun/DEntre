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
#include <string.h>

#include "globals.h"
#include "utils.h"
#include "fragment.h"
#include "vmareas.h"
#include "heap.h"
#include "link.h"
#include "mips/proc.h"


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
}free_list_header_t;


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

	struct _fcache_t *cache;
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



/* one "code cache" of a single type of fragment, made up of potentially
 * multiple FcacheUnits
 */
typedef struct _fcache_t
{
	bool is_trace:1;
	bool is_shared:1;
#ifdef DEBUG
	bool is_local;
#endif
	bool is_coarse:1;

	fragment_t *fifo;
	fcache_unit_t *units;
	size_t size;

	mutex_t lock;

#ifdef DEBUG
	char *name;
	bool consistent;
#endif

	/* backpointer for mapping cache pc to coarse info for inter-unit unlink */
	coarse_info_t *coarse_info;

    /* we cache parameter here so we don't have to dispatch on bb/trace
     * type every time -- this also allows flexibility if we ever want
     * to have different parameters per thread or something.
     * not much of a space hit at all since there are 2 caches per thread
     * and then 2 global caches.
     */
	uint max_size;
	uint max_unit_size;
	uint max_quadrupled_unit_size;
	uint free_upgrade_size;
	uint init_unit_size;
	bool finite_cache;
	uint regen_param;
	uint replace_param;

    /* for adaptive working set: */
    uint      num_regenerated;
    uint      num_replaced; /* for shared cache, simply number created */
    /* for fifo caches, wset_check is simply an optimization to avoid
     * too many checks when parameters are such that regen<<replace
     */
    int      wset_check;
    /* for non-fifo caches, this flag indicates we should start
     * recording num_regenerated and num_replaced
     */
    bool     record_wset;

	free_list_header_t *free_list[FREE_LIST_SIZES_NUM];

#ifdef DEBUG
    uint free_stats_freed[FREE_LIST_SIZES_NUM]; /* occurrences */
    uint free_stats_reused[FREE_LIST_SIZES_NUM]; /* occurrences */
    uint free_stats_coalesced[FREE_LIST_SIZES_NUM]; /* occurrences */
    uint free_stats_split[FREE_LIST_SIZES_NUM]; /* entry split, occurrences */
    uint free_stats_charge[FREE_LIST_SIZES_NUM]; /* bytes on free list */
    /* sizes of real requests and frees */
    uint request_size_histogram[HISTOGRAM_MAX_SIZE/HISTOGRAM_GRANULARITY];
    uint free_size_histogram[HISTOGRAM_MAX_SIZE/HISTOGRAM_GRANULARITY];
#endif
}fcache_t;


/* per-thread structure: 
 * FIXME: give a better name to distinguish from heap.c's _thread_units_t
 */
typedef struct _thread_units_t
{
	fcache_t *bb;
	fcache_t *trace;
	/* we delay unmapping units, but only one at a time: */
	cache_pc pending_unmap_pc;
	size_t pending_unmap_size;
	/* are there units waiting to be flushed at a safe spot? */
	bool pending_flush;
}thread_units_t;


#define PROTECT_CACHE(cache, op)	/* need to be filled up */

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

static fcache_t *shared_cache_bb;
static fcache_t *shared_cache_trace;

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



/* Pass NULL for pc if this routine should allocate the cache space.
 * If pc is non-NULL, this routine assumes that size is fully
 * committed and initializes accordingly.
 */
static fcache_unit_t *
fcache_creat_unit(dcontext_t *dcontext, fcache_t *cache, cache_pc pc, size_t size)
{
	/* need to be filled up */
}


/* to make it easy to switch to INTERNAL_OPTION */
#define FCACHE_OPTION(o) dentre_options.o

/* assuming size will either be aligned at VM_ALLOCATION_BOUNDARY or
 * smaller where no adjustment is necessary 
 */
#define FCACHE_GUARDED(size)                                    \
        ((size) -                                               \
         ((DENTRE_OPTION(guard_pages) &&                        \
           ((size) >= VM_ALLOCATION_BOUNDARY - 2 * PAGE_SIZE))  \
          ? (2 * PAGE_SIZE) : 0))


#define SET_CACHE_PARAMS(cache, which)				\
	do												\
	{												\
		cache->max_size = 							\
			FCACHE_GUARDED(FCACHE_OPTION(cache_##which##_max));				\
		cache->max_unit_size =                                              \
		    FCACHE_GUARDED(FCACHE_OPTION(cache_##which##_unit_max));        \
		cache->max_quadrupled_unit_size =                                   \
		    FCACHE_GUARDED(FCACHE_OPTION(cache_##which##_unit_quadruple));  \
		cache->free_upgrade_size =                                          \
		    FCACHE_GUARDED(FCACHE_OPTION(cache_##which##_unit_upgrade));    \
		cache->init_unit_size =                                             \
		    FCACHE_GUARDED(FCACHE_OPTION(cache_##which##_unit_init));       \
		cache->finite_cache = dentre_options.finite_##which##_cache;        \
		cache->regen_param = dentre_options.cache_##which##_regen;          \
		cache->replace_param = dentre_options.cache_##which##_replace;      \
	}while(0);		

static fcache_t *
fcache_cache_init(dcontext_t *dcontext, uint flags, bool initial_unit)
{
	fcache_t *cache = (fcache_t *)
		nonpersistent_heap_alloc(dcontext, sizeof(fcache_t) HEAPACCT(ACCT_MEM_MGT));
	cache->fifo = NULL;
	cache->size = 0;
	cache->is_trace = TEST(FRAG_IS_TRACE, flags);
	cache->is_shared = TEST(FRAG_SHARED, flags);
	cache->is_coarse = TEST(FRAG_COARSE_GRAIN, flags);
	DODEBUG({ cache->is_local = false; });
	cache->coarse_info = NULL;
	DODEBUG({ cache->consistent = true; });

	if(cache->is_shared)
	{
		ASSERT(dcontext == GLOBAL_DCONTEXT);
		if(cache->is_trace)
		{
			DODEBUG({ cache->name = "Trace (shared)"; });
			SET_CACHE_PARAMS(cache, shared_trace);
		}
		else if(cache->is_coarse)
		{
			DODEBUG({ cache->name = "Coarse basic block (shared)"; });
			SET_CACHE_PARAMS(cache, coarse_bb);
		}
		else
		{
			DODEBUG({ cache->name = "Basic block (shared)"; });
			SET_CACHE_PARAMS(cache, shared_bb);
		}
	}
	else
	{
		ASSERT(dcontext != GLOBAL_DCONTEXT);
		if(cache->is_trace)
		{
			DODEBUG({ cache->name = "Trace (private)"; });
			SET_CACHE_PARAMS(cache, trace);
		}
		else
		{
			DODEBUG({ cache->name = "Basic block (private)"; });
			SET_CACHE_PARAMS(cache, bb);
		}
	}

#ifdef DISALLOW_CACHE_RESIZING
    /* cannot handle resizing of cache, separate units only */
    cache->init_unit_size = cache->max_unit_size;
#endif
	
	if(cache->is_shared)
		ASSIGN_INIT_LOCK_FREE(cache->lock, shared_cache_lock);

	if(initial_unit)
	{
		PROTECT_CACHE(cache, lock);
		/* 64k for shared bb or N64, 4k for local bb*/
		/* 64k for shared trace or N64, 8k for local trace*/
		cache->units = fcache_creat_unit(dcontext, cache, NULL, cache->init_unit_size);
		PROTECT_CACHE(cache, unlock);
	}
	else
	{
		cache->units = NULL;
	}

    cache->num_regenerated = 0;
    cache->num_replaced = 0;
    cache->wset_check = 0;
    cache->record_wset = false;

    if (cache->is_shared) { /* else won't use free list */
        memset(cache->free_list, 0, sizeof(cache->free_list));
        DODEBUG({
            memset(cache->free_stats_freed, 0, sizeof(cache->free_stats_freed));
            memset(cache->free_stats_reused, 0, sizeof(cache->free_stats_reused));
            memset(cache->free_stats_coalesced, 0, sizeof(cache->free_stats_coalesced));
            memset(cache->free_stats_charge, 0, sizeof(cache->free_stats_charge));
            memset(cache->free_stats_split, 0, sizeof(cache->free_stats_split));
            memset(cache->request_size_histogram, 0, 
                   sizeof(cache->request_size_histogram));
            memset(cache->free_size_histogram, 0, 
                   sizeof(cache->free_size_histogram));
        });
    }

	return cache;
}

/* thread-shared initialization that should be repeated after a reset */
static void
fcache_reset_init(void)
{
    /* case 7966: don't initialize at all for hotp_only & thin_client
     * FIXME: could set initial sizes to 0 for all configurations, instead
     */
	if(RUNNING_WITHOUT_CODE_CACHE())
		return;

	if(DENTRE_OPTION(shared_bbs))
	{
		shared_cache_bb = fcache_cache_init(GLOBAL_DCONTEXT, FRAG_SHARED, true);
		ASSERT(shared_cache_bb != NULL);
        LOG(GLOBAL, LOG_CACHE, 1, "Initial shared bb cache is %d KB\n",
            shared_cache_bb->init_unit_size/1024);
	}

	if(DENTRE_OPTION(shared_traces))
	{
		shared_cache_trace = fcache_cache_init(GLOBAL_DCONTEXT, FRAG_SHARED|FRAG_IS_TRACE, true);
		ASSERT(shared_cache_trace != NULL);
        LOG(GLOBAL, LOG_CACHE, 1, "Initial shared trace cache is %d KB\n",
            shared_cache_bb->init_unit_size/1024);
	}
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


static void 
fcache_thread_reset_init(dcontext_t *dcontext)
{
	/* nothing */
}

void
fcache_thread_init(dcontext_t *dcontext)
{
	thread_units_t *tu = (thread_units_t *) heap_alloc(dcontext, sizeof(thread_units_t)
												HEAPACCT(ACCT_OTHER));
	dcontext->fcache_field = (void *)tu;
    /* don't build trace cache until we actually build a trace
     * this saves memory for both DENTRE_OPTION(disable_traces) and for
     * idle threads that never do much
     */
	tu->trace = NULL;
    /* in fact, let's delay both, cost is single conditional in fcache_add_fragment,
     * once we have that conditional for traces it's no extra cost for bbs
     */
	tu->bb = NULL;
	tu->pending_unmap_pc = NULL;
	tu->pending_flush = false;

	fcache_thread_reset_init(dcontext);
}
